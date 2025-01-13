#pragma once
#include <string>
#include <plog/Log.h>
#include "Tribes-Ascend-SDK/SdkHeaders.h"

#define GWORLD_ADDRESS (0x013cb664)
#define GENGINE_ADDRESS (0x14232800)

extern bool isClient;
extern ATrPlayerController* myPlayerController;
extern UWorld* GWorld;

std::string FVectorToString(const FVector& vector);

bool IsInFieldOfView(const FVector& location, FVector direction, const FVector& position, float* OUT_dot = nullptr, float* OUT_angle = nullptr);

std::vector<FVector> GetLocationOfLineIntersectionWithCircleXY(const FVector& lineStart, const FVector& lineEnd,
															   const FVector& circleOrigin, const float circleRadii, double* OUT_a = nullptr, double* OUT_b = nullptr, double* OUT_c = nullptr,
															   double* OUT_discriminant = nullptr, double* OUT_t1 = nullptr, double* OUT_t2 = nullptr);
inline bool IsPawnValid(ATrPlayerPawn* pawn)
{
	if (pawn && pawn->PlayerReplicationInfo && pawn->Health)
	{
		return true;
	}
	return false;
}

template< class T >
std::vector<T*> GetInstancesUObjects(void)
{
	/*while (!UObject::GObjObjects())
		Sleep(100);

	while (!FName::Names())
		Sleep(100);*/

	std::vector<T*> foundUObjects;

	for (int i = 0; i < UObject::GObjObjects()->Count; ++i)
	{
		UObject* Object = UObject::GObjObjects()->Data[i];

		// skip no T class objects 
		if
			(
				!Object
				|| !Object->IsA(T::StaticClass())
				)
			continue;

		// check 
		foundUObjects.push_back(reinterpret_cast<T*>(Object));
	}

	return foundUObjects;
}