#include <format>
#include <plog/Log.h>
#include "Helper.hpp"
#include "LagCompensation.hpp"

bool IsPawnValid(ATrPlayerPawn* pawn)
{
    if (pawn && pawn->PlayerReplicationInfo && pawn->Health)
    {
        return true;
    }
    PLOG_WARNING << "Pawn is no longer valid";
    return false;
}

ATrPlayerController* GetProjectileController(ATrProjectile* Projectile)
{
    // Only return a non null value if the controller is a player controller
    // Things such as turrets can fire projectiles, which we do not want to lag compensate
    if (Projectile->InstigatorController && Projectile->InstigatorController->IsA(ATrPlayerController::StaticClass()))
    {
        return reinterpret_cast<ATrPlayerController*>(Projectile->InstigatorController);
    }
    PLOG_ERROR << "Projectile is not owned by a player";
    return nullptr;
}

float GetProjectilePingInMS(ATrProjectile* Projectile)
{
    auto controller{ GetProjectileController(Projectile) };
    if (controller)
    {
        // If we're in a debug build and DEBUG_PING is non zero then forcefully set the ping
#ifdef _DEBUG
        return DEBUG_PING >= 0 ? DEBUG_PING : controller->PlayerReplicationInfo->ExactPing * 4;
#else
        return controller->PlayerReplicationInfo->ExactPing * 4;
#endif
    }
    PLOG_ERROR << "Projectile controller ping is invalid";
    return -1;
}