#include <format>
#include <plog/Log.h>
#include "ProcessInternalHooks.hpp"

void __fastcall TribesGame_TrProjectile_Explode_Hook(UObject* CallingUObject,
													 void* Unused, FFrame& Stack,
													 void* Result)
{
	PLOG_INFO << __FUNCTION__ << " called";
	auto gameProjectile = reinterpret_cast<ATrProjectile*>(CallingUObject);
	projectiles.erase(gameProjectile);
}