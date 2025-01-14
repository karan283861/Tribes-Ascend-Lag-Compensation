#include <format>
#include <unordered_map>
#include <plog/Log.h>
#include "NativeHooks.hpp"
#include "LagCompensation.hpp"
#include "Helper.hpp"

// During an engine tick, MoveActor is called for all actors
// In this case, projectiles are moved after all players have been moved
// This boolean is used to report if MoveActor is called on a projectile
// meaning all players have finished moving
static auto playersMoved{ false };

UWorldMoveActorPrototype OriginalUWorldMoveActor = reinterpret_cast<UWorldMoveActorPrototype>(UWORLD_MOVEACTOR_ADDRESS);
bool __fastcall UWorld_MoveActor_Hook(UWorld* World, void* Unused,
                                      AActor* Actor,
                                      const FVector& Delta,
                                      const FRotator& NewRotation,
                                      unsigned int MoveFlags,
                                      FCheckHitResult& Hit)
{
    PLOG_DEBUG << __FUNCTION__;

    if (Actor->IsA(ATrPlayerPawn::StaticClass()))
    {
        // Move the player
        auto returnValue{ OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit) };
        auto playerPawn{ reinterpret_cast<ATrPlayerPawn*>(Actor) };
        // Save the latest player state into the latest lag compensation tick
        // MoveActor may be called multiple times so we want to insert of assign in case
        // the player and player state is already mapped
        latestLagCompensationTick.m_PlayerToLocation.insert_or_assign(playerPawn, playerPawn->Location);
        return returnValue;
    }
    else if (Actor->IsA(ATrProjectile::StaticClass()))
    {
        if (!playersMoved)
        {
            // All players have been moved
            // Populate the lag compensation buffer with the latest lag compensation tick
            // to get the 0 ping states of players
            playersMoved = true;
            lagCompensationBuffer.push_back(latestLagCompensationTick);
        }

        auto gameProjectile{ reinterpret_cast<ATrProjectile*>(Actor) };

        auto pingInMS{ GetProjectilePingInMS(gameProjectile) };
        if (pingInMS < 0)
        {
            // This should only happen if the controller of the projectile is not a player controller
            // or if something went very wrong...
            return OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit);
        }

        auto lagCompensationTick{ GetLagCompensationTick(pingInMS) };
        auto previousLagCompensationTick{ GetPreviousLagCompensationTick(pingInMS) };
        if (lagCompensationTick && previousLagCompensationTick)
        {
            // Everything seems ok - We have a valid ping and two ticks the projectile is between
            // Move players back in time
            RewindPlayers(pingInMS);
            // Now move the projectile
            auto returnValue{ OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit) };
            // Restore the players to their latest positions
            RestorePlayers(pingInMS);
            return returnValue;
        }
    }

    return OriginalUWorldMoveActor(World, Unused, Actor, Delta, NewRotation, MoveFlags, Hit);
}

UGameEngineTickPrototype OriginalUGameEngineTick = reinterpret_cast<UGameEngineTickPrototype>(UGAMEENGINE_TICK_ADDRESS);
void __fastcall UGameEngine_Tick_Hook(UGameEngine* GameEngine, void* Unused, float DeltaSeconds)
{
    PLOG_DEBUG << __FUNCTION__;
    // Fresh engine tick
    // No projectiles have moved
    playersMoved = false;
    // Clear latest lag compensation tick
    latestLagCompensationTick = LagCompensationTick();
    // Call engine tick, which calls MoveActor which will populate latestLagCompensationTick
    OriginalUGameEngineTick(GameEngine, Unused, DeltaSeconds);
    if (!playersMoved)
    {
        // MoveActor for a projectile was not called in this engine tick
        // This means playersMoved is still false, however players
        // would still have moved. Populate the lag compensation buffer
        // with the latest lag compensation tick to get the 0 ping states of players
        lagCompensationBuffer.push_back(latestLagCompensationTick);
    }
}