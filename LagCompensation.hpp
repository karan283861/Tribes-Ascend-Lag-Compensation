#include <unordered_map>
#include "Tribes-Ascend-SDK/SdkHeaders.h"

class Projectile
{
	//bool m_Valid{};
	ATrProjectile* m_Projectile{};
	FVector m_Location{};
	FVector m_PreviousLocation{};
	bool m_IsArcing{};
	static float CollisionScalar;
public:
	Projectile(ATrProjectile* projectile);
};

extern std::unordered_map<ATrProjectile*, Projectile> projectiles;