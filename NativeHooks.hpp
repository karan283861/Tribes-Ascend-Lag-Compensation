#pragma once

#include "Tribes-Ascend-SDK/SdkHeaders.h"

#define UWORLD_MOVEACTOR_ADDRESS (0x007e43d0)
#define UGAMEENGINE_TICK_ADDRESS (0x00790530)

class FCheckHitResult {};

using UWorldMoveActorPrototype = bool(__fastcall*)(UWorld* World, void* Unused,
                                                   AActor* Actor,
                                                   const FVector& Delta,
                                                   const FRotator& NewRotation,
                                                   unsigned int	MoveFlags,
                                                   FCheckHitResult& Hit);
extern UWorldMoveActorPrototype OriginalUWorldMoveActor;

bool __fastcall UWorld_MoveActor_Hook(UWorld* World, void* Unused,
                                      AActor* Actor,
                                      const FVector& Delta,
                                      const FRotator& NewRotation,
                                      unsigned int MoveFlags,
                                      FCheckHitResult& Hit);

using UGameEngineTickPrototype = void(__fastcall*)(UGameEngine* GameEngine, void* Unused, float DeltaSeconds);
extern UGameEngineTickPrototype OriginalUGameEngineTick;
void __fastcall UGameEngine_Tick_Hook(UGameEngine* GameEngine, void* Unused, float DeltaSeconds);
