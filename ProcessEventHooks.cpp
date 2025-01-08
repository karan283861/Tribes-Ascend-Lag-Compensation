#include <format>
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
}

void __fastcall TribesGame_TrProjectile_PostBeginPlay_Hook(UObject* CallingUObject,
														   void* Unused, UFunction* CallingUFunction,
														   void* Parameters, void* Result)
{
	PLOG_INFO << __FUNCTION__ << " called";
	auto gameProjectile = reinterpret_cast<ATrProjectile*>(CallingUObject);
	auto pawn = reinterpret_cast<ATrPlayerPawn*>(gameProjectile->Owner);
	auto controller = reinterpret_cast<ATrPlayerController*>(pawn->Owner);

	if (gameProjectile->IsA(ATrProj_Tracer::StaticClass()) || !pawn->IsA(ATrPlayerPawn::StaticClass()))
	{
		PLOG_INFO << "Invalid projectile";
		return;
	}

	Projectile projectile(gameProjectile);
	projectiles.emplace(gameProjectile, projectile);
}