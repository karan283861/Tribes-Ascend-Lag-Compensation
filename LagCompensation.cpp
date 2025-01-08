#include "LagCompensation.hpp"

float Projectile::CollisionScalar = 1.0;

Projectile::Projectile(ATrProjectile* projectile) : m_Projectile(projectile)
{
	//m_Valid = true;
	m_Location = projectile->Location;
	m_PreviousLocation = projectile->Location;
	m_IsArcing = projectile->CustomGravityScaling != 0 ? true : false;
}

std::unordered_map<ATrProjectile*, Projectile> projectiles;