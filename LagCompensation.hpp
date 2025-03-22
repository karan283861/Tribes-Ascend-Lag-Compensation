#include <unordered_map>
#include "circularbuffer/circular_buffer.h"
#include "Tribes-Ascend-SDK/SdkHeaders.h"

// If we're in a debug build and DEBUG_PING is non zero then use DEBUG_PING as the
// ping value. Obviously DEBUG_PING must be greater than zero, and less than
// LagCompensationWindowInMs otherwise the projectile will not be in the lag compensation buffer
#ifdef _DEBUG
#define DEBUG_PING (300.0f)
#endif

// Tick rate - This can be modified externally, initialise to the default value of 30
static float TickRate{ 30.0f };
static float TickDeltaInMS{ 1000.0f / TickRate };
static float LagCompensationWindowInMs{ 2000.0f };
static unsigned int LagCompensationBufferSize{ static_cast<unsigned int>(/*ceil*/(LagCompensationWindowInMs / TickDeltaInMS) + 1) };

class LagCompensationTick
{
public:
    // Map the location of each pawn in this tick
    std::unordered_map<ATrPlayerPawn*, FVector> m_PlayerToLocation{};
};

// The lag compesation buffer - every tick it stores the states players alive in said tick
extern CircularBuffer<LagCompensationTick> lagCompensationBuffer;
// Store the latest (0 ping) lag compensation tick
extern LagCompensationTick latestLagCompensationTick;
// Get a pointer to the lag compensation tick corresponding to the parameter ping
// Assuming the ping is valid, then the returned lag compensation tick object will have a ping
// less than (by a maximum of TickDeltaMS) or equal to the parameter ping
LagCompensationTick* GetLagCompensationTick(float PingInMS);
// Get a pointer to the previous lag compensation tick corresponding to the parameter ping
// Assuming the ping is valid, then the returned lag compensation tick object will have a ping
// greater than (by a maximum of TickDeltaMS) or equal to the parameter ping
LagCompensationTick* GetPreviousLagCompensationTick(float PingInMS);
// Move players back in time depending on the ping of a projectile
void RewindPlayers(float PingInMS);
// Move players forward in time to their location corresponding to latest tick
void RestorePlayers(float PingInMS);
// Update lag compensation tick related variables
void UpdateTickVariables(void);
