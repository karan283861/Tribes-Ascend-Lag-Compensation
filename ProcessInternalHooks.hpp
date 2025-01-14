#pragma once

#include "Hook.hpp"
#include "Tribes-Ascend-SDK/SdkHeaders.h"

void __fastcall TrProjectile_HurtRadius_Internal_Hook(UObject* CallingUObject,
                                                      void* Unused, FFrame& Stack,
                                                      void* Result);
