#pragma once

#include "Hook.hpp"
#include "Tribes-Ascend-SDK/SdkHeaders.h"

#define PROCESSINTERNAL_HOOK(functionHookName) void __fastcall functionHookName (UObject* CallingUObject, void* Unused, FFrame& Stack, void* Result)

PROCESSINTERNAL_HOOK(TrProjectile_HurtRadius_Internal_Hook);
PROCESSINTERNAL_HOOK(UTGame_MatchInProgress_BeginState_Hook);