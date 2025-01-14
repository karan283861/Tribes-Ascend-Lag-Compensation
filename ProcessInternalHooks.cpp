#include <format>
#include <plog/Log.h>
#include "ProcessInternalHooks.hpp"
#include "LagCompensation.hpp"
#include "Helper.hpp"

void __fastcall TrProjectile_HurtRadius_Internal_Hook(UObject* CallingUObject,
                                                      void* Unused,
                                                      FFrame& Stack,
                                                      void* Result)
{
    PLOG_DEBUG << __FUNCTION__;

    // The logic here is the same as the UWorld::MoveActor hook for a projectile
    // with one exception

    auto gameProjectile{ reinterpret_cast<ATrProjectile*>(CallingUObject) };
    auto controller{ GetProjectileController(gameProjectile) };

    if (!controller)
    {
        // The controller of the projectile is not a player controller
        return OriginalProcessInternalFunction(CallingUObject, Unused, Stack, Result);
    }

    auto pingInMS{ GetProjectilePingInMS(gameProjectile) };
    if (pingInMS < 0)
    {   // This should only happen if the controller of the projectile is not a player controller
        // or if something went very wrong...
        return OriginalProcessInternalFunction(CallingUObject, Unused, Stack, Result);
    }

    auto lagCompensationTick{ GetLagCompensationTick(pingInMS) };
    auto previousLagCompensationTick{ GetPreviousLagCompensationTick(pingInMS) };
    if (lagCompensationTick && previousLagCompensationTick)
    {
        // The exception : We do not want to rewind the owning player of the projectile
        // as this will mess with disc jumps/impulse damage
        auto playerPawn{ reinterpret_cast<ATrPlayerPawn*>(controller->Pawn) };
        auto playerPawnIsValid{ IsPawnValid(playerPawn) };
        // Store the location of the owning player of the projectile
        FVector playerPawnLocation{};

        if (playerPawnIsValid)
        {
            playerPawnLocation = playerPawn->Location;
        }
        RewindPlayers(pingInMS);
        if (playerPawnIsValid)
        {
            // After rewinding, restore the owning player of the projectile to its latest state
            playerPawn->SetLocation(playerPawnLocation);
        }
        // Apply radial damage
        OriginalProcessInternalFunction(CallingUObject, Unused, Stack, Result);
        RestorePlayers(pingInMS);
        return;
    }

    return OriginalProcessInternalFunction(CallingUObject, Unused, Stack, Result);
}