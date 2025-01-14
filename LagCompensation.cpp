#include <format>
#include <plog/Log.h>
#include "LagCompensation.hpp"
#include "Helper.hpp"

CircularBuffer<LagCompensationTick> lagCompensationBuffer(LagCompensationBufferSize);
LagCompensationTick latestLagCompensationTick{};

LagCompensationTick* GetLagCompensationTick(float PingInMS)
{
    if (lagCompensationBuffer.size() != LagCompensationBufferSize)
    {
        // Thue lag compensation buffer hasn't been filled up yet
        PLOG_WARNING << std::format("lagCompensationBuffer is not filled. Current size is {0}, Expected size is {1}",
                                    lagCompensationBuffer.size(), LagCompensationBufferSize);
        return nullptr;
    }

    auto lagCompensationTickIndex{ lagCompensationBuffer.size() - (int)ceil(PingInMS / TickDeltaInMS) };
    if (lagCompensationTickIndex >= lagCompensationBuffer.size() || lagCompensationTickIndex <= 0)
    {
        // The tick index corresponding to the parameter ping is outside of the bounds of the lag compensation buffer
        // This means the ping is greater than the lag compensation window or possibly negative
        PLOG_WARNING << std::format("lagCompensationTickIndex ({0}) is out of bounds", lagCompensationTickIndex);
        return nullptr;
    }
    return &lagCompensationBuffer.at(lagCompensationTickIndex);
}


LagCompensationTick* GetPreviousLagCompensationTick(float PingInMS)
{
    // The previous tick would just be the sum of parameter ping and TickDeltaInMS
    return GetLagCompensationTick(PingInMS + TickDeltaInMS);
}


void RewindPlayers(float PingInMS)
{
    auto lagCompensationTick{ GetLagCompensationTick(PingInMS) };
    auto previousLagCompensationTick{ GetPreviousLagCompensationTick(PingInMS) };

    if (!lagCompensationTick || !previousLagCompensationTick)
    {
        // Couldn't find any lag compensation ticks corresponding to the ping
        // Reason 1: lag compensation buffer is not full
        return;
    }

    // Loop over all the players in the lag compensation tick
    for (auto& [gamePawn, pawnLocation] : lagCompensationTick->m_PlayerToLocation)
    {
        // Check 1: latestLagCompensationTick.m_PlayerToLocation.contains(gamePawn) to ensure the player was alive in
        // the latest tick
        // Check 2: IsPawnValid(gamePawn) to ensure the player is still alive - it could have previously died from a
        // different projectile in the same engine tick
        // Check 3: previousLagCompensationTick->m_PlayerToLocation.contains(gamePawn) to ensure the player was alive
        // in the previous tick so we can interpolate
        if (latestLagCompensationTick.m_PlayerToLocation.contains(gamePawn) && IsPawnValid(gamePawn) &&
            previousLagCompensationTick->m_PlayerToLocation.contains(gamePawn))
        {
            // Lets say the ping of the player who shot this projectile is 120.0 ms (ie. PingInMs = 120.0)
            // Assume the default value of TickDeltaInMS to be 33.33 ms
            // The current lag compensation tick will have a ping of ~ 100 ms (33.33 * 3)
            // The previous lag compensation tick will have a ping of ~ 133 ms (33.33 * 4)
            // So, we interpolate the player location to between 100 ms and 133 ms to match the 120 ms ping
            // (120 % 33.33) ~ 20 ms -> (20 ms / 33.33 ms) ~ 0.6 ->
            // interpolatedLocation = location + (previousLocation - location) * 0.6
            float millisecondRemainder{ fmod(PingInMS, TickDeltaInMS) };
            // A vector towards the previous location
            auto delta{ gamePawn->Subtract_VectorVector(previousLagCompensationTick->m_PlayerToLocation.at(gamePawn),
                                                        pawnLocation) };
            // Scale delta by how close our ping remainder is to TickDeltaInMS
            // The closer the ping is to the previous lag compensation tick, the closer this scalar is to 1.0
            float interpolateScalar{ millisecondRemainder / TickDeltaInMS }; // Between 0.0 - 1.0
            // TODO: Implement these vector calculations myself or hook the native functions in the engine
            // that perform these calculations. Reduce usage of Unreal VM
            // Move backwards depending on interpolateScalar
            auto interpolatedLocation{ gamePawn->Add_VectorVector(pawnLocation,
                                                                  gamePawn->Multiply_VectorFloat(delta, interpolateScalar)) };
            // TODO: Hook the native functions in the engine that performs this functionality
            // Reduce usage of Unreal VM
            // Maybe even set the interal Location vector directly (perhaps requires modifying
            // the collision component too)
            gamePawn->SetLocation(interpolatedLocation);
        }
    }
}

void RestorePlayers(float PingInMS)
{
    auto lagCompensationTick{ GetLagCompensationTick(PingInMS) };

    if (!lagCompensationTick)
    {
        return;
    }

    for (auto& [gamePawn, pawnLocation] : lagCompensationTick->m_PlayerToLocation)
    {
        // Read RewindPlayers function to understand the reason for these checks
        if (latestLagCompensationTick.m_PlayerToLocation.contains(gamePawn) && IsPawnValid(gamePawn))
        {
            // TODO: Hook the native functions in the engine that performs this functionality
            // Reduce usage of Unreal VM
            // Maybe even set the interal Location vector directly (perhaps requires modifying
            // the collision component too)
            gamePawn->SetLocation(latestLagCompensationTick.m_PlayerToLocation.at(gamePawn));
        }
    }
}