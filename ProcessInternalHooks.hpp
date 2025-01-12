#include "Hook.hpp"
#include "LagCompensation.hpp"

void __fastcall TribesGame_TrProjectile_InitProjectile_Hook(UObject* CallingUObject,
															void* Unused, FFrame& Stack,
															void* Result);

void __fastcall TribesGame_TrProjectile_Explode_Hook(UObject* CallingUObject,
													 void* Unused, FFrame& Stack,
													 void* Result);