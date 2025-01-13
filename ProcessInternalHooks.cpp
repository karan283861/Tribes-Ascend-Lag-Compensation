#include <format>
#include <plog/Log.h>
#include "ProcessInternalHooks.hpp"
#include "Helper.hpp"

void __fastcall TribesGame_TrProjectile_InitProjectile_Hook(UObject* CallingUObject,
															void* Unused, FFrame& Stack,
															void* Result)
{
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

void __fastcall TribesGame_TrProjectile_Explode_Hook(UObject* CallingUObject,
													 void* Unused, FFrame& Stack,
													 void* Result)
{
	PLOG_DEBUG << __FUNCTION__ << " called";
	/*if (isClient)
	return;*/
	auto gameProjectile = reinterpret_cast<ATrProjectile*>(CallingUObject);
	if (projectiles.find(gameProjectile) != projectiles.end())
	{
		projectiles.at(gameProjectile).m_Valid = false;
	}
	//projectiles.erase(gameProjectile);
}

void __fastcall TrProjectile_HurtRadius_Internal_Hook(UObject* CallingUObject,
													  void* Unused, FFrame& Stack,
													  void* Result)
{
	if (isClient)
	{
		return OriginalProcessInternalFunction(CallingUObject, Unused, Stack, Result);
	}


	auto gameProjectile{ reinterpret_cast<ATrProjectile*>(CallingUObject) };
	auto pawn = reinterpret_cast<ATrPlayerPawn*>(gameProjectile->Owner);
	auto controller = reinterpret_cast<ATrPlayerController*>(pawn->Owner);

	auto pingInMS{ controller->PlayerReplicationInfo->ExactPing * 4 };
#ifdef _DEBUG
	if (DEBUG_PING != 0)
	{
		pingInMS = DEBUG_PING;
	}
#endif

	auto lagCompensationTick = GetLagCompensationTick(pingInMS);
	auto previousLagCompensationTick = GetPreviousLagCompensationTick(pingInMS);
	if (lagCompensationTick && previousLagCompensationTick)
	{
		MovePawns(pingInMS);
		OriginalProcessInternalFunction(CallingUObject, Unused, Stack, Result);
		RestorePawns(pingInMS);
		return;
	}

	return OriginalProcessInternalFunction(CallingUObject, Unused, Stack, Result);
}