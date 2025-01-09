#include "LagCompensation.hpp"

float Projectile::CollisionScalar = 0.315;

Projectile::Projectile(ATrProjectile* projectile) : m_GameProjectile(projectile)
{
	m_SpawnTimestamp = projectile->CreationTime;
	//m_Valid = true;
	//m_Location = projectile->Location;
	m_PreviousLocation = projectile->Location;
	m_InitialVelocity = projectile->Velocity;
	//m_IsArcing = projectile->CustomGravityScaling != 0 ? true : false;
	auto pawn = reinterpret_cast<ATrPlayerPawn*>(projectile->Owner);
	auto controller = reinterpret_cast<ATrPlayerController*>(projectile->Owner);

	m_PingInMS = controller->PlayerReplicationInfo->ExactPing * 4 + 200;
}

PlayerInformation::PlayerInformation(ATrPlayerPawn* pawn) : m_Pawn(pawn)
{
	//m_Controller = reinterpret_cast<ATrPlayerController*>(pawn->Owner);
	m_Location = pawn->Location;
	//m_Velocity = pawn->Velocity;
}

std::unordered_map<ATrProjectile*, Projectile> projectiles{};
CircularBuffer<LagCompensationTick> lagCompensationBuffer(LagCompensationBufferSize);