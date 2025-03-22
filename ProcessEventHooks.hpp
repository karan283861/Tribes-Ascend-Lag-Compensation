#pragma once

#include "Hook.hpp"
#include "Tribes-Ascend-SDK/SdkHeaders.h"

#define PROCESSEVENT_HOOK(functionHookName) void __fastcall functionHookName (UObject* CallingUObject, void* Unused, UFunction* CallingUFunction, void* Parameters, void* Result)