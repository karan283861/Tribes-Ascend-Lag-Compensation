#include <format>
#include <unordered_set>
#include <plog/Log.h>
#include "ProcessEventHooks.hpp"
#include "LagCompensation.hpp"

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
		auto c{ 0 };
		for (auto& [gameProjectile, projectile] : projectiles)
		{
			PLOG_DEBUG << std::format("{0}:\tX = {1}\tY = {2}\tZ = {3}",
									  c,
									  gameProjectile->Location.X,
									  gameProjectile->Location.Y,
									  gameProjectile->Location.Z);
			c++;
		}
	}

	auto gameReplicationInfo = reinterpret_cast<ATrGameReplicationInfo*>(CallingUObject);
	auto worldInfo = gameReplicationInfo->WorldInfo;
	auto pawns = worldInfo->PawnList;
	std::unordered_set<APawn*> alivePawns;

	LagCompensationTick lagCompensationTick;
	lagCompensationTick.m_Timestamp = worldInfo->TimeSeconds;

	while (pawns)
	{
		auto pawn = reinterpret_cast<ATrPlayerPawn*>(pawns);
		if (pawn && pawn->PlayerReplicationInfo && pawn->Health)
		{
			auto& playerInformation = PlayerInformation(pawn);
			lagCompensationTick.m_PlayerIDToPlayerInformation.emplace(playerInformation.GetID(), playerInformation);
			alivePawns.insert(pawn);
		}
		pawns = pawn->NextPawn;
	}

	lagCompensationBuffer.push_back(lagCompensationTick);

	if (lagCompensationBuffer.size() != LagCompensationBufferSize)
		return;

	for (auto& [gameProjectile, projectile] : projectiles)
	{
		const auto& uobject = gameProjectile;
		bool hit{};
		static FVector hitLocation, hitNormal;
		static FTraceHitInfo hitInfo;

		for (auto& [gameProjectile, projectile] : projectiles)
		{
			auto lagCompensationTickIndex = lagCompensationBuffer.size() - (int)ceil(projectile.m_PingInMS / TickDeltaInMS);
			if (lagCompensationTickIndex >= lagCompensationBuffer.size() || lagCompensationTickIndex < 1)
				continue;

			auto& lagCompensationTick = lagCompensationBuffer.at(lagCompensationTickIndex);
			auto& previousLagCompensationTick = lagCompensationBuffer.at(lagCompensationTickIndex - 1);

			for (auto& [playerID, currentPlayerInformation] : lagCompensationTick.m_PlayerIDToPlayerInformation)
			{
				if (alivePawns.find(currentPlayerInformation.m_Pawn) == alivePawns.end())
					continue;

				if (!previousLagCompensationTick.m_PlayerIDToPlayerInformation.contains(playerID))
					continue;

				auto& previousPlayerInformation = previousLagCompensationTick.m_PlayerIDToPlayerInformation.find(playerID)->second;

				auto deltaTimeOfPingAndTickInMS = (projectile.m_PingInMS) - (lagCompensationTick.m_Timestamp - worldInfo->TimeSeconds) * 1000;
				auto playerLocationInterpolated = uobject->Add_VectorVector(currentPlayerInformation.m_Location,
																			(uobject->Multiply_VectorFloat(uobject->Subtract_VectorVector(previousPlayerInformation.m_Location, currentPlayerInformation.m_Location),
																										   deltaTimeOfPingAndTickInMS / TickDeltaInMS)));

				if (projectile.GetOwningController()->Trace(currentPlayerInformation.m_Location, gameProjectile->Location,
															true, FVector(), false, &hitLocation, &hitNormal, &hitInfo) != currentPlayerInformation.m_Pawn)
					continue;

				auto DoesProjectileIntersectPlayer = [uobject](FVector projectileLocation, float projectileRadius, FVector playerLocation, float playerRadius)
					{
						auto magnitude = uobject->VSize2D(uobject->Subtract_VectorVector(playerLocation, projectileLocation));
						return magnitude < (playerRadius + projectileRadius);
					};

				auto projectileStartPoint = projectile.m_PreviousLocation;
				auto projectileEndPoint = gameProjectile->Location;
			}

			projectile.m_PreviousLocation = gameProjectile->Location;
		}

	}
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
		PLOG_WARNING << "Projectile ping out of window";
		return;
	}

	projectiles.emplace(gameProjectile, projectile);
}