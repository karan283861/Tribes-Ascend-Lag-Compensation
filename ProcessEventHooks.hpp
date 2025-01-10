#include "Hook.hpp"
#include "Tribes-Ascend-SDK/SdkHeaders.h"

void __fastcall Engine_HUD_PostRender_Hook(UObject* CallingUObject,
										   void* Unused, UFunction* CallingUFunction,
										   void* Parameters, void* Result);

void __fastcall TribesGame_TrGameReplicationInfo_Tick_Hook(UObject* CallingUObject,
														   void* Unused, UFunction* CallingUFunction,
														   void* Parameters, void* Result);

void __fastcall TribesGame_TrProjectile_PostBeginPlay_Hook(UObject* CallingUObject,
														   void* Unused, UFunction* CallingUFunction,
														   void* Parameters, void* Result);

void __fastcall TribesGame_TrHUD_PostRenderFor_Hook(UObject* CallingUObject,
													void* Unused, UFunction* CallingUFunction,
													void* Parameters, void* Result);