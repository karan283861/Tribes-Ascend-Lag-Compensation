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