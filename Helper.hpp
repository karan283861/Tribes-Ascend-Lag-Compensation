#pragma once
#include "Tribes-Ascend-SDK/SdkHeaders.h"

// Check if a player is valid... this is, check if it's alive
bool IsPawnValid(ATrPlayerPawn* pawn);
// Get the player controller of a projectile
ATrPlayerController* GetProjectileController(ATrProjectile* Projectile);
// Get the ping of a projectile (through its player controller)
float GetProjectilePingInMS(ATrProjectile* Projectile);

// Get all instances of a specific UObject type in the GObjects buffer
template< class T >
std::vector<T*> GetInstancesUObjects(void)
{
    std::vector<T*> foundUObjects;

    for (int i = 0; i < UObject::GObjObjects()->Count; ++i)
    {
        UObject* Object = UObject::GObjObjects()->Data[i];
        if (!Object || !Object->IsA(T::StaticClass()))
            continue;

        foundUObjects.push_back(reinterpret_cast<T*>(Object));
    }
    return foundUObjects;
}