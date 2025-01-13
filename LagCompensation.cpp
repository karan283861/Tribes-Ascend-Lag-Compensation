#include "LagCompensation.hpp"

float Projectile::CollisionScalar = 0.333 * 1;//0.315;

Projectile::Projectile(ATrProjectile* projectile) : m_GameProjectile(projectile)
{
	//m_Timestamp = projectile->CreationTime;
	//m_Valid = true;
	//m_Location = projectile->Location;
	m_PreviousLocation = projectile->Location;
	m_InitialVelocity = projectile->Velocity;
	m_InitialLocation = projectile->Location;
	//m_IsArcing = projectile->CustomGravityScaling != 0 ? true : false;
	auto pawn = reinterpret_cast<ATrPlayerPawn*>(projectile->Owner);
	auto controller = reinterpret_cast<ATrPlayerController*>(pawn->Owner);

	m_PingInMS = controller->PlayerReplicationInfo->ExactPing * 4;
#ifdef _DEBUG
	if (DEBUG_PING != 0)
	{
		m_PingInMS = DEBUG_PING;
	}
#endif
}

PlayerInformation::PlayerInformation(ATrPlayerPawn* pawn) : m_Pawn(pawn)
{
	//m_Controller = reinterpret_cast<ATrPlayerController*>(pawn->Owner);
	m_Location = pawn->Location;
	//m_Velocity = pawn->Velocity;
}

std::unordered_map<ATrProjectile*, Projectile> projectiles{};
std::unordered_map<ATrProjectile*, float> projectileToPingInMS{};
CircularBuffer<LagCompensationTick> lagCompensationBuffer(LagCompensationBufferSize);
std::unordered_map<ATrPlayerPawn*, FVector> pawnToLocation{};

LagCompensationTick* GetLagCompensationTick(float PingInMS)
{
	if (lagCompensationBuffer.size() != LagCompensationBufferSize)
	{
		PLOG_WARNING << std::format("lagCompensationBuffer is not filled. Current size is {0}, Expected size is {1}",
									lagCompensationBuffer.size(), LagCompensationBufferSize);
		return nullptr;
	}

	auto lagCompensationTickIndex{ lagCompensationBuffer.size() - (int)ceil(PingInMS / TickDeltaInMS) };
	if (lagCompensationTickIndex >= lagCompensationBuffer.size() || lagCompensationTickIndex <= 0)
	{
		PLOG_WARNING << std::format("lagCompensationTickIndex ({0}) is out of bounds", lagCompensationTickIndex);
		return nullptr;
	}
	return &lagCompensationBuffer.at(lagCompensationTickIndex);
}


LagCompensationTick* GetPreviousLagCompensationTick(float PingInMS)
{
	return GetLagCompensationTick(PingInMS + TickDeltaInMS);
}

FVector GetInterpolatedLocationOfPawn(LagCompensationTick* lagCompensationTick, LagCompensationTick* previousLagCompensationTick,
									  float PingInMS, ATrPlayerPawn* Pawn)
{
	return lagCompensationTick->m_PawnToLocation.at(Pawn);
}

void MovePawns(float PingInMS)
{
	auto lagCompensationTick{ GetLagCompensationTick(PingInMS) };
	auto previousLagCompensationTick{ GetPreviousLagCompensationTick(PingInMS) };

	if (!lagCompensationTick || !previousLagCompensationTick)
	{
		return;
	}

	for (auto& [gamePawn, pawnLocation] : lagCompensationTick->m_PawnToLocation)
	{
		if (pawnToLocation.contains(gamePawn) && IsPawnValid(gamePawn) && previousLagCompensationTick->m_PawnToLocation.contains(gamePawn))
		{
			auto interpolatedLocation = GetInterpolatedLocationOfPawn(lagCompensationTick, previousLagCompensationTick,
																	  PingInMS, gamePawn);
			gamePawn->SetLocation(interpolatedLocation);
		}
	}
}

void RestorePawns(float PingInMS)
{
	auto lagCompensationTick{ GetLagCompensationTick(PingInMS) };

	if (!lagCompensationTick)
	{
		return;
	}

	for (auto& [gamePawn, pawnLocation] : lagCompensationTick->m_PawnToLocation)
	{
		if (pawnToLocation.contains(gamePawn) && IsPawnValid(gamePawn))
		{
			gamePawn->SetLocation(pawnToLocation.at(gamePawn));
		}
	}
}