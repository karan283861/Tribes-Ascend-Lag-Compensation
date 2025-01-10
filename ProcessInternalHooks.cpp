#include <format>
#include <plog/Log.h>
#include "ProcessInternalHooks.hpp"
#include "Helper.hpp"

void __fastcall TribesGame_TrProjectile_Explode_Hook(UObject* CallingUObject,
													 void* Unused, FFrame& Stack,
													 void* Result)
{
	if (isClient)
		return;

	PLOG_DEBUG << __FUNCTION__ << " called";
	auto gameProjectile = reinterpret_cast<ATrProjectile*>(CallingUObject);
	if (projectiles.find(gameProjectile) != projectiles.end())
	{
		projectiles.at(gameProjectile).m_Valid = false;
	}
	//projectiles.erase(gameProjectile);
}