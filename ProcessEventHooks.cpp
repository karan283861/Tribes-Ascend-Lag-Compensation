#include <format>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <plog/Log.h>
#include "ProcessEventHooks.hpp"
#include "LagCompensation.hpp"

std::string FVectorToString (const FVector& vector)
{
	return std::format("X = {0}\tY = {1}\tZ = {2}", vector.X, vector.Y, vector.Z);
};

void __fastcall Engine_HUD_PostRender_Hook(UObject* CallingUObject,
	void* Unused, UFunction* CallingUFunction,
	void* Parameters, void* Result)
{
	PLOG_INFO << __FUNCTION__ << " called";
	auto hud = reinterpret_cast<AHUD*>(CallingUObject);
	hud->Draw2DLine(0, 0, 1000, 1000, FColor(255, 255, 255, 255));
}

void __fastcall TribesGame_TrGameReplicationInfo_Tick_Hook(UObject* CallingUObject,
	void* Unused, UFunction* CallingUFunction,
	void* Parameters, void* Result)
{
	PLOG_INFO << __FUNCTION__ << " called";

	IF_PLOG(plog::debug)
	{
		//auto c{ 0 };
		for (auto& [gameProjectile, projectile] : projectiles)
		{
			PLOG_DEBUG << std::format("{0}:\tX = {1}\tY = {2}\tZ = {3}",
				static_cast<void*>(gameProjectile),
				gameProjectile->Location.X,
				gameProjectile->Location.Y,
				gameProjectile->Location.Z);
			//c++;
		}
	}

	auto gameReplicationInfo{ reinterpret_cast<ATrGameReplicationInfo*>(CallingUObject) };
	auto worldInfo{ gameReplicationInfo->WorldInfo };
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

	if (lagCompensationBuffer.size() != LagCompensationBufferSize)
	{
		PLOG_INFO << std::format("lagCompensationBuffer is not filled. Current size is {0}, Expected size is {1}",
			lagCompensationBuffer.size(), LagCompensationBufferSize);
		return;
	}

	std::unordered_set<ATrProjectile*> invalidProjectiles{};

	for (auto& [gameProjectile, projectile] : projectiles)
	{
		PLOG_DEBUG << std::format("{0}:\tPing = {1}", static_cast<void*>(gameProjectile), projectile.m_PingInMS);

		if (!projectile.GetOwningPawn() || !projectile.GetOwningController())
		{
			PLOG_ERROR << "Projectile has invalid owners (possibly destroyed?)";
			invalidProjectiles.insert(gameProjectile);
			continue;
		}

		const auto& uobject{ gameProjectile };
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

		PLOG_INFO << std::format("lagCompensationTick deltaTime (MS) = {0}", lagCompensationTickDeltaTimeInMS);
		PLOG_INFO << std::format("previousLagCompensationTick deltaTime (MS) = {0}", previousLagCompensationTickDeltaTimeInMS);

		if (lagCompensationTickDeltaTimeInMS > projectile.m_PingInMS || previousLagCompensationTickDeltaTimeInMS < projectile.m_PingInMS)
		{
			PLOG_ERROR << std::format("Incorrect lagCompensationTickIndex ({0}) chosen",
				lagCompensationTickIndex);
			continue;
		}
		//}

		auto deltaTimeOfPingAndTickInMS{ (projectile.m_PingInMS) - (worldInfo->TimeSeconds - lagCompensationTick.m_Timestamp) * 1000 };
		PLOG_INFO << std::format("deltaTimeOfPingAndTickInMS = {0}", deltaTimeOfPingAndTickInMS);

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
			auto& previousPlayerInformation{ previousLagCompensationTick.m_PawnToPlayerInformation.find(gamePawn)->second };

			if (deltaTimeOfPingAndTickInMS > TickDeltaInMS)
			{
				PLOG_ERROR << std::format("deltaTimeOfPingAndTickInMS ({0}) is greater than TickDeltaInMS ({1})",
					deltaTimeOfPingAndTickInMS, TickDeltaInMS);
				continue;
			}

			auto playerLocationInterpolated{ uobject->Add_VectorVector(currentPlayerInformation.m_Location,
				(uobject->Multiply_VectorFloat(uobject->Subtract_VectorVector(previousPlayerInformation.m_Location, currentPlayerInformation.m_Location),
					deltaTimeOfPingAndTickInMS / TickDeltaInMS))) };

			if (projectile.GetOwningController()->Trace(currentPlayerInformation.m_Location, gameProjectile->Location,
				true, FVector(), false, &hitLocation, &hitNormal, &hitInfo) != currentPlayerInformation.m_Pawn)
			{
				PLOG_DEBUG << std::format("{0} does not have LOS to {1}", static_cast<void*>(gameProjectile), static_cast<void*>(gamePawn));
				continue; // Uncomment this
			}

			auto IsInFieldOfView = [uobject](const FVector& location, FVector direction, const FVector& position)
				{
					direction.Z = 0;
					auto difference{ uobject->Subtract_VectorVector(position, location) };
					difference.Z = 0;
					auto dot{ uobject->Dot_VectorVector(direction, difference) };
					return dot > 0;
				};

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

			auto projectileToPlayerXY{ uobject->Subtract_VectorVector(playerLocationInterpolated, gameProjectile->Location) };
			projectileToPlayerXY.Z = 0;
			auto projectileVelocityXY{ gameProjectile->Velocity };
			projectileVelocityXY.Z = 0;
			auto closestPointInProjectilePathToPlayer{ uobject->ProjectOnTo(projectileToPlayerXY /*u*/, projectileVelocityXY /*v*/)}; /*proj_v^u*/
			//auto closestPointToPlayer{ uobject->ProjectOnTo(projectileVelocityXY, projectileToPlayerXY) };
			auto vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer{ uobject->Normal(uobject->Subtract_VectorVector(playerLocationInterpolated, closestPointInProjectilePathToPlayer)) };
			vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer.Z = 0;

			auto projectileRadii{ std::min(gameProjectile->CheckRadius * Projectile::CollisionScalar, gamePawn->CylinderComponent->CollisionRadius) };
			auto playerRadii{ std::max(gameProjectile->CheckRadius * Projectile::CollisionScalar, gamePawn->CylinderComponent->CollisionRadius) };

			auto projectileLineStartPoint{ uobject->Add_VectorVector(gameProjectile->Location, uobject->Multiply_VectorFloat(vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer, projectileRadii)) };
			auto projectileLineEndPoint{ uobject->Add_VectorVector(projectile.m_PreviousLocation, uobject->Multiply_VectorFloat(vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer, projectileRadii)) };

			//bool matchXY{ false };
			auto isProjectileLineStartPointInPlayer{ DoesProjectileIntersectPlayer(projectileLineStartPoint, projectileRadii,
				playerLocationInterpolated, playerRadii) };
			auto isProjectileLineEndPointInPlayer{ DoesProjectileIntersectPlayer(projectileLineEndPoint, projectileRadii,
				playerLocationInterpolated, playerRadii) };

			auto GetLocationOfLineIntersectionWithCircleXY = [uobject](const FVector& lineStart, const FVector& lineEnd,
																		const FVector& circleOrigin, const float circleRadii)
				{
					auto a{ pow((lineEnd.X - lineStart.X), 2) + pow((lineEnd.Y - lineStart.Y), 2) };
					auto b{ 2 * (lineEnd.X - lineStart.X) * (lineStart.X - circleOrigin.X) + 2 * (lineEnd.Y - lineStart.Y) * (lineStart.Y - circleOrigin.Y) };
					auto c{ pow(lineStart.X - circleOrigin.X, 2) + pow(lineStart.Y - circleOrigin.Y, 2) - pow(circleRadii, 2) };
					PLOG_INFO << std::format("a = {0}\tb = {1}\tc = {2}", a, b, c);
					std::vector<FVector> intersections{};
					auto det{ pow(b, 2) - 4 * a * c };
					PLOG_INFO << std::format("Det = {0}", det);
					static const float TINY_NUMBER{ 1E-5 };
					if (det < 0)
					{
						return intersections;
					}
					else if (det > 0 && det < TINY_NUMBER)
					{
						det = 0;
					}

					auto t1{ (-b - sqrt(det)) / (2 * a) };
					auto t2{ (-b + sqrt(det)) / (2 * a) };

					PLOG_INFO << std::format("t1 = {0}\tt2={1}", t1, t2);

					if (t1 >= 0 && t1 <= 1)
					{
						auto intersectionLocation = uobject->Add_VectorVector(lineStart, uobject->Multiply_VectorFloat(uobject->Subtract_VectorVector(lineEnd, lineStart), t1));
						intersections.push_back(intersectionLocation);
					}

					if (det == 0)
						return intersections;

					if (t2 >= 0 && t2 <= 1)
					{
						auto intersectionLocation = uobject->Add_VectorVector(lineStart, uobject->Multiply_VectorFloat(uobject->Subtract_VectorVector(lineEnd, lineStart), t2));
						intersections.push_back(intersectionLocation);
					}

					return intersections;
				};

			IF_PLOG(plog::info)
			{
				PLOG_INFO << std::format("gameProjectile->Location ({0}):\t{1}", static_cast<void*>(gameProjectile), FVectorToString(gameProjectile->Location));
				PLOG_INFO << std::format("projectile.m_PreviousLocation:\t{0}", FVectorToString(projectile.m_PreviousLocation));
				PLOG_INFO << std::format("gamePawn->Location ({0}):\t{1}", static_cast<void*>(gamePawn), FVectorToString(gamePawn->Location));
				PLOG_INFO << std::format("playerLocationInterpolated:\t{0}", FVectorToString(playerLocationInterpolated));
				PLOG_INFO << std::format("closestPointInProjectilePathToPlayer:\t{0}", FVectorToString(closestPointInProjectilePathToPlayer));
				PLOG_INFO << std::format("vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer:\t{0}", FVectorToString(vectorFromClosestPointInProjectilePathToPlayerTowardsPlayer));
				PLOG_INFO << std::format("playerRadii:\t{0}", playerRadii);
				PLOG_INFO << std::format("projectileRadii:\t{0}", projectileRadii);
				PLOG_INFO << std::format("projectileLineStartPoint:\t{0}", FVectorToString(projectileLineStartPoint));
				PLOG_INFO << std::format("projectileLineEndPoint:\t{0}", FVectorToString(projectileLineEndPoint));
				PLOG_INFO << std::format("isProjectileLineStartPointInPlayer:\t{0}", isProjectileLineStartPointInPlayer);
				PLOG_INFO << std::format("isProjectileLineEndPointInPlayer:\t{0}", isProjectileLineEndPointInPlayer);

				PLOG_INFO << std::format("Unit gameProjectile->Velocity\t{0}", FVectorToString(uobject->Normal(gameProjectile->Velocity)));
				PLOG_INFO << std::format("Unit closestPointInProjectilePathToPlayer\t{0}", FVectorToString(uobject->Normal(closestPointInProjectilePathToPlayer)));
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
				IF_PLOG(plog::error)
				{
					if (intersections.size() != 1)
					{
						PLOG_ERROR << "Incorrect number of intersections found between projectile line and player";
						continue;
					}
				}
				projectileLineStartPoint = intersections[0];
			}
			else if (isProjectileLineStartPointInPlayer && !isProjectileLineEndPointInPlayer)
			{
				PLOG_INFO << "Projectile starts inside player";
				auto intersections{ GetLocationOfLineIntersectionWithCircleXY(projectileLineStartPoint, projectileLineEndPoint, playerLocationInterpolated, playerRadii) };
				IF_PLOG(plog::error)
				{
					if (intersections.size() != 1)
					{
						PLOG_ERROR << "Incorrect number of intersections found between projectile line and player";
						continue;
					}
				}
				projectileLineEndPoint = intersections[0];
			}
			else if (!isProjectileLineStartPointInPlayer && !isProjectileLineEndPointInPlayer)
			{
				PLOG_DEBUG << "Projectile starts and ends outside player";
				auto intersections{ GetLocationOfLineIntersectionWithCircleXY(projectileLineStartPoint, projectileLineEndPoint, playerLocationInterpolated, playerRadii) };
				IF_PLOG(plog::error)
				{
					if (intersections.size() != 2)
					{
						PLOG_ERROR << "Incorrect number of intersections found between projectile line and player";
						continue;
					}
				}
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
			}
			else
			{
				PLOG_INFO << std::format("{0} does not collide with {1}", static_cast<void*>(gameProjectile), static_cast<void*>(gamePawn));
			}
		}

		projectile.m_PreviousLocation = gameProjectile->Location;
	}

	for (auto& gameProjectile : invalidProjectiles)
	{
		projectiles.erase(gameProjectile);
	}

	invalidProjectiles.clear();
}

void __fastcall TribesGame_TrProjectile_PostBeginPlay_Hook(UObject* CallingUObject,
	void* Unused, UFunction* CallingUFunction,
	void* Parameters, void* Result)
{
	PLOG_INFO << __FUNCTION__ << " called";
	auto gameProjectile = reinterpret_cast<ATrProjectile*>(CallingUObject);
	auto pawn = reinterpret_cast<ATrPlayerPawn*>(gameProjectile->Owner);
	auto controller = reinterpret_cast<ATrPlayerController*>(pawn->Owner);

	if (gameProjectile->IsA(ATrProj_Tracer::StaticClass()) || gameProjectile->IsA(ATrProj_ClientTracer::StaticClass()) || !pawn->IsA(ATrPlayerPawn::StaticClass()))
	{
		PLOG_INFO << "Invalid projectile";
		return;
	}

	Projectile projectile(gameProjectile);

	if (projectile.m_PingInMS > LagCompensationWindowInMs)
	{
		PLOG_WARNING << "Projectile ping out of lag compensation window";
		return;
	}

	projectiles.emplace(gameProjectile, projectile);
}