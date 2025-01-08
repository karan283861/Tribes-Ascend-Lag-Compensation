#include <unordered_map>
#include <plog/Log.h>
#include "circularbuffer/circular_buffer.h"
#include "Tribes-Ascend-SDK/SdkHeaders.h"

static const float TickRate = 30.0;
static const float TickDeltaInMS = 1000.0 / TickRate;
static const float LagCompensationWindowInMs = 1000.0;
static const unsigned int LagCompensationBufferSize = (unsigned int)/*ceil*/(LagCompensationWindowInMs / TickDeltaInMS) + 1;

using PlayerID = unsigned int;

class Projectile
{
public:
	//bool m_Valid{};
	float m_SpawnTimestamp{};
	ATrProjectile* m_GameProjectile{};
	FVector m_PreviousLocation{};
	FVector m_InitialVelocity{};
	//bool m_IsArcing{};
	float m_PingInMS{};
	static float CollisionScalar;

	Projectile(ATrProjectile* projectile);
	
	ATrPlayerPawn* GetOwningPawn(void)
	{
		PLOG_DEBUG << m_GameProjectile->GetFullName();
		return reinterpret_cast<ATrPlayerPawn*>(m_GameProjectile->Owner);
	}

	ATrPlayerController* GetOwningController(void)
	{
		return reinterpret_cast<ATrPlayerController*>(GetOwningPawn()->Owner);
	}
};

class PlayerInformation
{
public:
	ATrPlayerPawn* m_Pawn{};
	//ATrPlayerController* m_Controller{};
	FVector m_Location{};
	FVector m_Velocity{};

	PlayerInformation(ATrPlayerPawn* pawn);

	unsigned int GetID(void)
	{
		return m_Pawn->PlayerReplicationInfo->UniqueId.Uid.A;
	}

	ATrPlayerController* GetOwningController(void)
	{
		return reinterpret_cast<ATrPlayerController*>(m_Pawn->Owner);
	}
};

class LagCompensationTick
{
public:
	float m_Timestamp{};
	std::unordered_map<PlayerID, PlayerInformation> m_PlayerIDToPlayerInformation{};
};

extern std::unordered_map<ATrProjectile*, Projectile> projectiles;
extern CircularBuffer<LagCompensationTick> lagCompensationBuffer;