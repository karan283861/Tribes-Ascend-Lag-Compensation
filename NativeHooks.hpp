#pragma once

#include "Tribes-Ascend-SDK/SdkHeaders.h"

#define MOVEACTOR_ADDRESS (0x008ee2d0)
#define SETLOCATION_ADDRESS (0x008ebfc0)
#define UWORLD_MOVEACTOR_ADDRESS (0x007e43d0)
#define UGAMEENGINE_TICK_ADDRESS (0x00790530)
#define UWORLD_TICK_ADDRESS (0x0080bee0)

using AActorMovePrototype = bool(__fastcall*)(AActor* Actor, void* Unused, FVector Delta);
extern AActorMovePrototype OriginalAActorMove;
bool __fastcall AActor_Move_Hook(AActor* Actor, void* Unused, FVector Delta);

using AActorSetLocationPrototype = bool(__fastcall*)(AActor* Actor, void* Unused, FVector NewLocation);
extern AActorSetLocationPrototype OriginalAActorSetLocation;
bool __fastcall AActor_SetLocation_Hook(AActor* Actor, void* Unused, FVector NewLocation);

using UWorldMoveActorPrototype = bool(__fastcall*)(UWorld* World, void* Unused,
												   AActor* Actor,
												   const FVector& Delta,
												   const FRotator& NewRotation,
												   unsigned int			MoveFlags,
												   int& Hit);
extern UWorldMoveActorPrototype OriginalUWorldMoveActor;
bool __fastcall UWorld_MoveActor_Hook(UWorld* World, void* Unused,
									  AActor* Actor,
									  const FVector& Delta,
									  const FRotator& NewRotation,
									  unsigned int			MoveFlags,
									  int& Hit);

using UGameEngineTickPrototype = void(__fastcall*)(UGameEngine* GameEngine, void* Unused, float DeltaSeconds);
extern UGameEngineTickPrototype OriginalUGameEngineTick;
void __fastcall UGameEngine_Tick_Hook(UGameEngine* GameEngine, void* Unused, float DeltaSeconds);

using UWorldTickPrototype = void(__fastcall*)(UWorld* World, void* Unused, unsigned int TickType, float DeltaSeconds);
extern UWorldTickPrototype OriginalUWorldTick;
void __fastcall UWorld_Tick_Hook(UWorld* World, void* Unused, unsigned int TickType, float DeltaSeconds);
