//#define NOMINMAX
//#include <Windows.h>
#include <format>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <plog/Log.h>
#include "ProcessEventHooks.hpp"
#include "LagCompensation.hpp"
#include "Helper.hpp"

void __fastcall Engine_HUD_PostRender_Hook(UObject* CallingUObject,
	void* Unused, UFunction* CallingUFunction,
	void* Parameters, void* Result)
{
	PLOG_DEBUG << __FUNCTION__ << " called";
	auto hud = reinterpret_cast<AHUD*>(CallingUObject);
	hud->Draw2DLine(0, 0, 1000, 1000, FColor(255, 255, 255, 255));
}

void __fastcall TribesGame_TrGameReplicationInfo_Tick_Hook(UObject* CallingUObject,
	void* Unused, UFunction* CallingUFunction,
	void* Parameters, void* Result)
{
	PLOG_DEBUG << __FUNCTION__ << " called";


	IF_PLOG(plog::debug)
	{
		//auto c{ 0 };
		for (auto& [gameProjectile, projectile] : projectiles)
		{
			PLOG_DEBUG << std::format("game Projetile ({0}) Location:{1}", static_cast<void*>(gameProjectile),
									  FVectorToString(gameProjectile->Location));
			//c++;
		}
	}

	auto gameReplicationInfo{ reinterpret_cast<ATrGameReplicationInfo*>(CallingUObject) };
	auto worldInfo{ gameReplicationInfo->WorldInfo };
	static auto previousWorldTimeSeconds{ worldInfo->TimeSeconds };
	auto deltaWorldTimeSecondsInMS{ (worldInfo->TimeSeconds - previousWorldTimeSeconds) * 1000.0 };
	PLOG_DEBUG << std::format("deltaWorldTimeSecondsInMS = {0}", deltaWorldTimeSecondsInMS);
	previousWorldTimeSeconds = worldInfo->TimeSeconds;
	auto pawns{ worldInfo->PawnList };
	std::unordered_set<APawn*> alivePawns;

	LagCompensationTick lagCompensationTick;
	lagCompensationTick.m_Timestamp = worldInfo->TimeSeconds;

	while (pawns)
	{
		auto pawn = reinterpret_cast<ATrPlayerPawn*>(pawns);
		if (pawn && pawn->PlayerReplicationInfo && pawn->Health)
		{
			auto& playerInformation = PlayerInformation(pawn);
			lagCompensationTick.m_PawnToPlayerInformation.emplace(pawn, playerInformation);
			alivePawns.insert(pawn);
		}
		pawns = pawn->NextPawn;
	}

	lagCompensationBuffer.push_back(lagCompensationTick);

	if (isClient)
		return;

	if (lagCompensationBuffer.size() != LagCompensationBufferSize)
	{
		PLOG_INFO << std::format("lagCompensationBuffer is not filled. Current size is {0}, Expected size is {1}",
			lagCompensationBuffer.size(), LagCompensationBufferSize);
		return;
	}

	std::unordered_set<ATrProjectile*> invalidProjectiles{};

	for (auto& [gameProjectile, projectile] : projectiles)
	{
		const auto& uobject{ reinterpret_cast<UObject*>(gameProjectile) };

		PLOG_DEBUG << std::format("{0}:\tPing = {1}", static_cast<void*>(gameProjectile), projectile.m_PingInMS);

		if (!projectile.m_Valid || !projectile.GetOwningPawn() || !projectile.GetOwningController() || !uobject->VSize(gameProjectile->Velocity))
		{
			PLOG_ERROR << "Projectile has invalid data (possibly destroyed?)";
			invalidProjectiles.insert(gameProjectile);
			continue;
		}

		/*if (uobject->EqualEqual_VectorVector(gameProjectile->Location, projectile.m_PreviousLocation))
		{
			PLOG_WARNING << "Projectile does not appear to have moved";
			continue;
		}*/

		auto hit{ false };
		static FVector hitLocation{}, hitNormal{};
		static FTraceHitInfo hitInfo{};

		auto lagCompensationTickIndex{ lagCompensationBuffer.size() - (int)ceil(projectile.m_PingInMS / TickDeltaInMS) };
		if (lagCompensationTickIndex >= lagCompensationBuffer.size() || lagCompensationTickIndex <= 0)
		{
			PLOG_WARNING << std::format("lagCompensationTickIndex ({0}) is out of bounds", lagCompensationTickIndex);
			continue;
		}

		auto& lagCompensationTick{ lagCompensationBuffer.at(lagCompensationTickIndex) };
		auto& previousLagCompensationTick{ lagCompensationBuffer.at(lagCompensationTickIndex - 1) };

		//IF_PLOG(plog::error)
		//{
		auto lagCompensationTickDeltaTimeInMS{ (worldInfo->TimeSeconds - lagCompensationTick.m_Timestamp) * 1000 };
		auto previousLagCompensationTickDeltaTimeInMS{ (worldInfo->TimeSeconds - previousLagCompensationTick.m_Timestamp) * 1000 };

		PLOG_DEBUG << std::format("lagCompensationTick deltaTime (MS) = {0}", lagCompensationTickDeltaTimeInMS);
		PLOG_DEBUG << std::format("previousLagCompensationTick deltaTime (MS) = {0}", previousLagCompensationTickDeltaTimeInMS);

		if (lagCompensationTickDeltaTimeInMS > projectile.m_PingInMS || previousLagCompensationTickDeltaTimeInMS < projectile.m_PingInMS)
		{
			PLOG_ERROR << std::format("Incorrect lagCompensationTickIndex ({0}) chosen",
				lagCompensationTickIndex);
			//continue;
		}
		//}

		auto deltaTimeOfPingAndTickInMS{ (projectile.m_PingInMS) - (worldInfo->TimeSeconds - lagCompensationTick.m_Timestamp) * 1000 };
		PLOG_DEBUG << std::format("deltaTimeOfPingAndTickInMS = {0}", deltaTimeOfPingAndTickInMS);

		if (deltaTimeOfPingAndTickInMS > TickDeltaInMS)
		{
			PLOG_ERROR << std::format("deltaTimeOfPingAndTickInMS ({0}) is greater than TickDeltaInMS ({1})",
									  deltaTimeOfPingAndTickInMS, TickDeltaInMS);
			//continue;
		}

		auto projectileAliveTime{ worldInfo->TimeSeconds - gameProjectile->CreationTime };
		auto estimatedProjectileLocation{ uobject->Add_VectorVector(projectile.m_InitialLocation,
																	uobject->Multiply_VectorFloat(gameProjectile->Velocity, projectileAliveTime)) };

		PLOG_DEBUG << std::format("Projectile alive time = {0}", projectileAliveTime);
		auto projectileTravelTime{ uobject->VSize2D(uobject->Subtract_VectorVector(gameProjectile->Location, projectile.m_InitialLocation)) /
															   uobject->VSize2D(projectile.m_InitialVelocity)}; // only valid for non arc projectiles
		PLOG_DEBUG << std::format("projectileTravelTimeInMS = {0}", projectileTravelTime);
		//auto estimatedProjectileLocation{ gameProjectile->Location };

		for (auto& [gamePawn, currentPlayerInformation] : lagCompensationTick.m_PawnToPlayerInformation)
		{

			if (projectile.GetOwningPawn() == gamePawn)
			{
				PLOG_DEBUG << std::format("{0} is the owner of the projectile", static_cast<void*>(gamePawn));
				continue;
			}

			if (alivePawns.find(currentPlayerInformation.m_Pawn) == alivePawns.end())
			{
				PLOG_DEBUG << std::format("{0} is not currently alive", static_cast<void*>(gamePawn));
				continue;
			}

			if (!previousLagCompensationTick.m_PawnToPlayerInformation.contains(gamePawn))
			{
				PLOG_DEBUG << std::format("{0} is not stored in previousLagCompensationTick", static_cast<void*>(gamePawn));
				continue;
			}
			auto& previousPlayerInformation{ previousLagCompensationTick.m_PawnToPlayerInformation.at(gamePawn) };

			/*auto playerLocationInterpolated{ uobject->Add_VectorVector(currentPlayerInformation.m_Location,
				(uobject->Multiply_VectorFloat(uobject->Subtract_VectorVector(previousPlayerInformation.m_Location, currentPlayerInformation.m_Location),
					deltaTimeOfPingAndTickInMS / TickDeltaInMS))) };*/

			auto playerLocationInterpolated{ currentPlayerInformation.m_Location };

			//if (projectile.GetOwningController()->Trace(gamePawn->Location, estimatedProjectileLocation,
			//	true, FVector(), false, &hitLocation, &hitNormal, &hitInfo) != currentPlayerInformation.m_Pawn)
			//{
			//	PLOG_DEBUG << std::format("{0} does not have LOS to {1}", static_cast<void*>(gameProjectile), static_cast<void*>(gamePawn));
			//	continue; // Uncomment this
			//}

			// check if projectile.m_PreviousLocation is out of FOV of player
			if (!IsInFieldOfView(projectile.m_PreviousLocation, gameProjectile->Velocity, playerLocationInterpolated))
			{
				PLOG_DEBUG << std::format("{1} is outside the FOV of {0}", static_cast<void*>(gamePawn), static_cast<void*>(gameProjectile));
				continue;
			}

			PLOG_DEBUG << std::format("{0} intersection check against {1}", static_cast<void*>(gameProjectile), static_cast<void*>(gamePawn));

			auto DoesProjectileIntersectPlayer = [uobject](const FVector& projectileLocation, const float& projectileRadius,
				const FVector& playerLocation, const float& playerRadius)
				{
					auto magnitude = uobject->VSize2D(uobject->Subtract_VectorVector(playerLocation, projectileLocation));
					return magnitude < (playerRadius + projectileRadius);
				};

			auto projectileToPlayerXY{ uobject->Subtract_VectorVector(playerLocationInterpolated, estimatedProjectileLocation) };
			projectileToPlayerXY.Z = 0;
			auto projectileVelocityXY{ gameProjectile->Velocity };
			projectileVelocityXY.Z = 0;
			auto closestPointInProjectilePathToPlayer{ uobject->ProjectOnTo(projectileToPlayerXY /*u*/, projectileVelocityXY /*v*/) }; /*proj_v^u*/
			//auto closestPointToPlayer{ uobject->ProjectOnTo(projectileVelocityXY, projectileToPlayerXY) };
			auto vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer{ uobject->Normal(uobject->Subtract_VectorVector(playerLocationInterpolated, closestPointInProjectilePathToPlayer)) };
			vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer.Z = 0;

			auto projectileRadii{ std::min(gameProjectile->CheckRadius * Projectile::CollisionScalar, gamePawn->CylinderComponent->CollisionRadius * 1) };
			auto playerRadii{ std::max(gameProjectile->CheckRadius * Projectile::CollisionScalar, gamePawn->CylinderComponent->CollisionRadius * 1) };

			auto projectileLineStartPoint{ uobject->Add_VectorVector(projectile.m_PreviousLocation, uobject->Multiply_VectorFloat(vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer, projectileRadii)) };
			auto projectileLineEndPoint{ uobject->Add_VectorVector(estimatedProjectileLocation, uobject->Multiply_VectorFloat(vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer, projectileRadii)) };

			//bool matchXY{ false };
			auto isProjectileLineStartPointInPlayer{ DoesProjectileIntersectPlayer(projectileLineStartPoint, projectileRadii,
				playerLocationInterpolated, playerRadii) };
			auto isProjectileLineEndPointInPlayer{ DoesProjectileIntersectPlayer(projectileLineEndPoint, projectileRadii,
				playerLocationInterpolated, playerRadii) };

			IF_PLOG(plog::info)
			{
				PLOG_DEBUG << std::format("gameProjectile->Location ({0}):\t{1}", static_cast<void*>(gameProjectile), FVectorToString(gameProjectile->Location));
				PLOG_DEBUG << std::format("estimatedProjectileLocation:\t{0}", FVectorToString(estimatedProjectileLocation));
				PLOG_DEBUG << std::format("projectile.m_PreviousLocation:\t{0}", FVectorToString(projectile.m_PreviousLocation));
				PLOG_DEBUG << std::format("gamePawn->Location ({0}):\t{1}", static_cast<void*>(gamePawn), FVectorToString(gamePawn->Location));
				PLOG_DEBUG << std::format("currentPlayerInformation.m_Location ({0}):\t{1}", static_cast<void*>(gamePawn), FVectorToString(currentPlayerInformation.m_Location));
				PLOG_DEBUG << std::format("playerLocationInterpolated:\t{0}", FVectorToString(playerLocationInterpolated));
				PLOG_DEBUG << std::format("closestPointInProjectilePathToPlayer:\t{0}", FVectorToString(closestPointInProjectilePathToPlayer));
				PLOG_DEBUG << std::format("vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer:\t{0}", FVectorToString(vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer));
				PLOG_DEBUG << std::format("playerRadii:\t{0}", playerRadii);
				PLOG_DEBUG << std::format("projectileRadii:\t{0}", projectileRadii);
				PLOG_DEBUG << std::format("projectileLineStartPoint:\t{0}", FVectorToString(projectileLineStartPoint));
				PLOG_DEBUG << std::format("projectileLineEndPoint:\t{0}", FVectorToString(projectileLineEndPoint));
				PLOG_DEBUG << std::format("isProjectileLineStartPointInPlayer:\t{0}", isProjectileLineStartPointInPlayer);
				PLOG_DEBUG << std::format("isProjectileLineEndPointInPlayer:\t{0}", isProjectileLineEndPointInPlayer);

				PLOG_DEBUG << std::format("Unit gameProjectile->Velocity\t{0}", FVectorToString(uobject->Normal(gameProjectile->Velocity)));
				PLOG_DEBUG << std::format("Unit closestPointInProjectilePathToPlayer\t{0}", FVectorToString(uobject->Normal(closestPointInProjectilePathToPlayer)));
			}

			if (isProjectileLineStartPointInPlayer && isProjectileLineEndPointInPlayer)
			{
				PLOG_INFO << "Projectile starts and ends inside player";
				//matchXY = true;
			}
			else if (!isProjectileLineStartPointInPlayer && isProjectileLineEndPointInPlayer)
			{
				PLOG_INFO << "Projectile ends inside player";
				auto intersections{ GetLocationOfLineIntersectionWithCircleXY(projectileLineStartPoint, projectileLineEndPoint, playerLocationInterpolated, playerRadii) };
				/*IF_PLOG(plog::error)
				{*/
					if (intersections.size() != 1)
					{
						PLOG_ERROR << "Incorrect number of intersections found between projectile line and player";
						continue;
					}
				/*}*/
				projectileLineStartPoint = intersections[0];
			}
			else if (isProjectileLineStartPointInPlayer && !isProjectileLineEndPointInPlayer)
			{
				PLOG_INFO << "Projectile starts inside player";
				auto intersections{ GetLocationOfLineIntersectionWithCircleXY(projectileLineStartPoint, projectileLineEndPoint, playerLocationInterpolated, playerRadii) };
				/*IF_PLOG(plog::error)
				{*/
					if (intersections.size() != 1)
					{
						PLOG_ERROR << "Incorrect number of intersections found between projectile line and player";
						continue;
					}
				/*}*/
				projectileLineEndPoint = intersections[0];
			}
			else if (!isProjectileLineStartPointInPlayer && !isProjectileLineEndPointInPlayer)
			{
				PLOG_DEBUG << "Projectile starts and ends outside player";
				auto intersections{ GetLocationOfLineIntersectionWithCircleXY(projectileLineStartPoint, projectileLineEndPoint, playerLocationInterpolated, playerRadii) };
				/*IF_PLOG(plog::error)
				{*/
					if (intersections.size() != 2)
					{
						PLOG_ERROR << "Incorrect number of intersections found between projectile line and player";
						continue;
					}
				/*}*/
				projectileLineStartPoint = intersections[0];
				projectileLineEndPoint = intersections[1];
			}
			else
			{
				PLOG_ERROR << "Invalid case encountered";
				continue;
			}

			PLOG_INFO << std::format("projectileLineStartPoint:\t{0}", FVectorToString(projectileLineStartPoint));
			PLOG_INFO << std::format("projectileLineEndPoint:\t{0}", FVectorToString(projectileLineEndPoint));

			//hit = true;

			// Check line end points
			if (abs(projectileLineStartPoint.Z - playerLocationInterpolated.Z) < gameProjectile->CheckRadius * Projectile::CollisionScalar + gamePawn->CylinderComponent->CollisionHeight)
			{
				PLOG_INFO << "Projectile start point collides with player";
				hit = true;
			}
			if (abs(projectileLineEndPoint.Z - playerLocationInterpolated.Z) < gameProjectile->CheckRadius * Projectile::CollisionScalar + gamePawn->CylinderComponent->CollisionHeight)
			{
				PLOG_INFO << "Projectile end point collides with player";
				hit = true;
			}

			// Check line went through circle
			auto zAxisDistanceFromProjectileLineStartPointToPlayer = projectileLineStartPoint.Z - playerLocationInterpolated.Z;
			auto zAxisDistanceFromProjectileLineEndPointToPlayer = projectileLineEndPoint.Z - playerLocationInterpolated.Z;
			if (copysign(1, zAxisDistanceFromProjectileLineStartPointToPlayer) != copysign(1, zAxisDistanceFromProjectileLineEndPointToPlayer))
			{
				PLOG_INFO << "Projectile collides with player";
				hit = true;
			}

			if (hit)
			{
				PLOG_INFO << std::format("{0} does collide with {1}", static_cast<void*>(gameProjectile), static_cast<void*>(gamePawn));
				gameProjectile->SetLocation(gamePawn->Location);
				invalidProjectiles.insert(gameProjectile);
				break;
			}
			else
			{
				PLOG_INFO << std::format("{0} does not collide with {1}", static_cast<void*>(gameProjectile), static_cast<void*>(gamePawn));
			}
		}

		if (hit)
			continue;

		projectile.m_PreviousLocation = gameProjectile->Location;
	}

	for (auto& gameProjectile : invalidProjectiles)
	{
		projectiles.erase(gameProjectile);
	}

	invalidProjectiles.clear();
}

void __fastcall TribesGame_TrHUD_PostRenderFor_Hook(UObject* CallingUObject,
													void* Unused, UFunction* CallingUFunction,
													void* Parameters, void* Result)
{

	auto params = reinterpret_cast<ATrHUD_eventPostRenderFor_Parms*>(Parameters);
	auto playerController = reinterpret_cast<ATrPlayerController*>(params->PC);
	auto uobject{ reinterpret_cast<UObject*>(playerController) };

	if (!isClient)
	{
		isClient = true;
		myPlayerController = playerController;
		auto trGameEngines = GetInstancesUObjects<UTrGameEngine>();
		// Client must be capped to (1000/35 (TickDeltaInMS)) fps to render properly
		for (auto& gameEngine : trGameEngines)
		{
			gameEngine->bSmoothFrameRate = true;
			gameEngine->MaxSmoothedFrameRate = (1000.0 / TickDeltaInMS);
		}
	}

	if (!playerController->Pawn)
		return;

	FVector eyeLocation;
	FRotator eyeRotation;
	playerController->eventGetActorEyesViewPoint(&eyeLocation, &eyeRotation);
	auto direction{ uobject->Normal(uobject->GetRotatorAxis(eyeRotation, 0)) };

	playerController->DrawDebugLine(eyeLocation, uobject->Add_VectorVector(eyeLocation, uobject->Multiply_VectorFloat(direction, 10000)),
									0, 255, 255, 0);

	auto worldInfo{ playerController->WorldInfo };
	auto pawns{ worldInfo->PawnList };
	std::unordered_set<APawn*> alivePawns;

	while (pawns)
	{
		auto pawn = reinterpret_cast<ATrPlayerPawn*>(pawns);
		if (pawn && pawn->PlayerReplicationInfo && pawn->Health)
		{
			alivePawns.insert(pawn);
		}
		pawns = pawn->NextPawn;
	}

	auto pingInMS{ playerController->PlayerReplicationInfo->ExactPing * 4 };
#ifdef _DEBUG
	if (DEBUG_PING != 0)
	{
		pingInMS = DEBUG_PING;
	}
#endif

	auto lagCompensationTickIndex{ lagCompensationBuffer.size() - (int)ceil(pingInMS / TickDeltaInMS) };
	if (lagCompensationTickIndex >= lagCompensationBuffer.size() || lagCompensationTickIndex <= 0)
	{
		PLOG_WARNING << std::format("lagCompensationTickIndex ({0}) is out of bounds", lagCompensationTickIndex);
		return;
	}

	auto& lagCompensationTick{ lagCompensationBuffer.at(lagCompensationTickIndex) };
	auto& previousLagCompensationTick{ lagCompensationBuffer.at(lagCompensationTickIndex - 1) };

	//IF_PLOG(plog::error)
	//{
	auto lagCompensationTickDeltaTimeInMS{ (worldInfo->TimeSeconds - lagCompensationTick.m_Timestamp) * 1000 };
	auto previousLagCompensationTickDeltaTimeInMS{ (worldInfo->TimeSeconds - previousLagCompensationTick.m_Timestamp) * 1000 };

	PLOG_INFO << std::format("lagCompensationTick deltaTime (MS) = {0}", lagCompensationTickDeltaTimeInMS);
	PLOG_INFO << std::format("previousLagCompensationTick deltaTime (MS) = {0}", previousLagCompensationTickDeltaTimeInMS);

	if (lagCompensationTickDeltaTimeInMS > pingInMS || previousLagCompensationTickDeltaTimeInMS < pingInMS)
	{
		PLOG_ERROR << std::format("Incorrect lagCompensationTickIndex ({0}) chosen",
								  lagCompensationTickIndex);
		//return;
	}
	//}

	auto deltaTimeOfPingAndTickInMS{ (pingInMS)-(worldInfo->TimeSeconds - lagCompensationTick.m_Timestamp) * 1000 };
	PLOG_INFO << std::format("deltaTimeOfPingAndTickInMS = {0}", deltaTimeOfPingAndTickInMS);

	if (deltaTimeOfPingAndTickInMS > TickDeltaInMS)
	{
		PLOG_ERROR << std::format("deltaTimeOfPingAndTickInMS ({0}) is greater than TickDeltaInMS ({1})",
								  deltaTimeOfPingAndTickInMS, TickDeltaInMS);
		//return;
	}

	static FVector hitLocation{}, hitNormal{};
	static FTraceHitInfo hitInfo{};

	for (auto& [gamePawn, currentPlayerInformation] : lagCompensationTick.m_PawnToPlayerInformation)
	{
		if (playerController->Pawn == gamePawn)
		{
			PLOG_DEBUG << std::format("Skipping drawing ourself");
			continue;
		}

		if (alivePawns.find(currentPlayerInformation.m_Pawn) == alivePawns.end())
		{
			PLOG_DEBUG << std::format("{0} is not currently alive", static_cast<void*>(gamePawn));
			continue;
		}

		if (!previousLagCompensationTick.m_PawnToPlayerInformation.contains(gamePawn))
		{
			PLOG_DEBUG << std::format("{0} is not stored in previousLagCompensationTick", static_cast<void*>(gamePawn));
			continue;
		}
		auto& previousPlayerInformation{ previousLagCompensationTick.m_PawnToPlayerInformation.at(gamePawn) };

		/*auto playerLocationInterpolated{ uobject->Add_VectorVector(currentPlayerInformation.m_Location,
			(uobject->Multiply_VectorFloat(uobject->Subtract_VectorVector(previousPlayerInformation.m_Location, currentPlayerInformation.m_Location),
				deltaTimeOfPingAndTickInMS / TickDeltaInMS))) };*/

		auto playerLocationInterpolated{ currentPlayerInformation.m_Location };

		if (playerController->Trace(gamePawn->Location, eyeLocation,
									true, FVector(), false, &hitLocation, &hitNormal, &hitInfo) != currentPlayerInformation.m_Pawn)
		{
			PLOG_DEBUG << std::format("Player does not have LOS to {0}", static_cast<void*>(gamePawn));
			continue; // Uncomment this
		}

		// check if projectile.m_PreviousLocation is out of FOV of player
		if (!IsInFieldOfView(playerController->Pawn->Location, direction, playerLocationInterpolated))
		{
			PLOG_DEBUG << std::format("{0} is outside the FOV of Player", static_cast<void*>(gamePawn));
			continue;
		}

		playerController->DrawDebugBox(playerLocationInterpolated, gamePawn->GetCollisionExtent(), 255, 0, 0, 0);
	}
}