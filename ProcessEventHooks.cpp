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
	if (!isClient)
		return;

	PLOG_DEBUG << __FUNCTION__ << " called";

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

void __fastcall TribesGame_TrProjectile_PostBeginPlay(UObject* CallingUObject,
													  void* Unused, UFunction* CallingUFunction,
													  void* Parameters, void* Result)
{
	if (isClient)
	{
		return;
	}

	PLOG_DEBUG << __FUNCTION__ << " called";

	auto gameProjectile = reinterpret_cast<ATrProjectile*>(CallingUObject);
	auto pawn = reinterpret_cast<ATrPlayerPawn*>(gameProjectile->Owner);
	auto controller = reinterpret_cast<ATrPlayerController*>(pawn->Owner);

	if (isClient && controller != myPlayerController)
		return;

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

	auto pingInMS = controller->PlayerReplicationInfo->ExactPing * 4;
#ifdef _DEBUG
	if (DEBUG_PING != 0)
	{
		pingInMS = DEBUG_PING;
	}
#endif

	projectileToPingInMS.emplace(gameProjectile, pingInMS);
}