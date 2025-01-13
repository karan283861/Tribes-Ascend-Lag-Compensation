#include <unordered_map>
#include <format>
#include <plog/Log.h>
#include "circularbuffer/circular_buffer.h"
#include "Tribes-Ascend-SDK/SdkHeaders.h"
#include "Helper.hpp"

#ifdef _DEBUG
#define DEBUG_PING (0.0)//(1000.0)
#else
#define DEBUG_PING (0.0)
#endif

static const float TickRate = 30.0;
static const float TickDeltaInMS = 1000.0 / TickRate;
static const float LagCompensationWindowInMs = 2000.0;
static const unsigned int LagCompensationBufferSize = (unsigned int)/*ceil*/(LagCompensationWindowInMs / TickDeltaInMS) + 1;

//using PlayerID = unsigned int;

class Projectile
{
public:
	bool m_Valid{ true };
	//float m_Timestamp{};
	ATrProjectile* m_GameProjectile{};
	FVector m_PreviousLocation{};
	FVector m_InitialVelocity{};
	FVector m_InitialLocation{};
	//bool m_IsArcing{};
	float m_PingInMS{};
	static float CollisionScalar;

	Projectile(ATrProjectile* projectile);
	
	ATrPlayerPawn* GetOwningPawn(void)
	{
		IF_PLOG(plog::error)
		{
			if (!m_GameProjectile->Owner)
			{
				PLOG_ERROR << std::format("{0}: Projectile owning pawn is null", static_cast<void*>(m_GameProjectile));
			}
		}
		return reinterpret_cast<ATrPlayerPawn*>(m_GameProjectile->Owner);
	}

	ATrPlayerController* GdetOwningController(void)
	{
		IF_PLOG(plog::error)
		{
			if (!GetOwningPawn()->Owner)
			{
				PLOG_ERROR << std::format("{0}: Projectile owning controller is null", static_cast<void*>(m_GameProjectile));
			}
		}
		return reinterpret_cast<ATrPlayerController*>(GetOwningPawn()->Owner);
	}
};

class PlayerInformation
{
public:
	ATrPlayerPawn* m_Pawn{};
	//ATrPlayerController* m_Controller{};
	FVector m_Location{};
	//FVector m_Velocity{};

	PlayerInformation(ATrPlayerPawn* pawn);

	unsigned int GetID(void)
	{
		return m_Pawn->PlayerReplicationInfo->UniqueId.Uid.A;
	}

	ATrPlayerController* GetOwningController(void)
	{
		IF_PLOG(plog::error)
		{
			if (!m_Pawn->Owner)
			{
				PLOG_ERROR << std::format("{0}: Pawn owning controller is null", static_cast<void*>(m_Pawn));
			}
		}
		return reinterpret_cast<ATrPlayerController*>(m_Pawn->Owner);
	}
};

class LagCompensationTick
{
public:
	float m_Timestamp{};
	std::unordered_map<ATrPlayerPawn*, PlayerInformation> m_PawnToPlayerInformation{};
	std::unordered_map<ATrPlayerPawn*, FVector> m_PawnToLocation;
};

extern std::unordered_map<ATrProjectile*, Projectile> projectiles;
extern std::unordered_map<ATrProjectile*, float> projectileToPingInMS;
extern CircularBuffer<LagCompensationTick> lagCompensationBuffer;
extern std::unordered_map<ATrPlayerPawn*, FVector> pawnToLocation;


LagCompensationTick* GetLagCompensationTick(float PingInMS);
LagCompensationTick* GetPreviousLagCompensationTick(float PingInMS);

FVector GetInterpolatedLocationOfPawn(LagCompensationTick* lagCompensationTick, LagCompensationTick* previousLagCompensationTick,
									  float PingInMS, ATrPlayerPawn* Pawn);

void MovePawns(float PingInMS);

void RestorePawns(float PingInMS);