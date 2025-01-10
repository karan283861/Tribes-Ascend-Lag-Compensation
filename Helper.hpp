#pragma once
#include <string>
#include <plog/Log.h>
#include "Tribes-Ascend-SDK/SdkHeaders.h"

extern bool isClient;

std::string FVectorToString(const FVector& vector);

bool IsInFieldOfView(const FVector& location, FVector direction, const FVector& position, float* OUT_dot = nullptr, float* OUT_angle = nullptr);

std::vector<FVector> GetLocationOfLineIntersectionWithCircleXY(const FVector& lineStart, const FVector& lineEnd,
	const FVector& circleOrigin, const float circleRadii, double* OUT_a = nullptr, double* OUT_b = nullptr, double* OUT_c = nullptr,
	double* OUT_discriminant = nullptr, double* OUT_t1 = nullptr, double* OUT_t2 = nullptr);