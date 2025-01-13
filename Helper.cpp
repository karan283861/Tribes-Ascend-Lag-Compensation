#include <format>
#include "Helper.hpp"

bool isClient{ false };
ATrPlayerController* myPlayerController{ nullptr };
UWorld* GWorld{ *reinterpret_cast<UWorld**>(GWORLD_ADDRESS) };

std::string FVectorToString(const FVector& vector)
{
	return std::format("X = {0}\tY = {1}\tZ = {2}", vector.X, vector.Y, vector.Z);
};

bool IsInFieldOfView(const FVector& location, FVector direction, const FVector& position, float* OUT_dot, float* OUT_angle)
{
	auto uobject = UObject::GObjObjects()->Data[1];
	direction.Z = 0;
	auto difference{ uobject->Subtract_VectorVector(position, location) };
	difference.Z = 0;
	PLOG_DEBUG << std::format("Direction = {0}", FVectorToString(direction));
	PLOG_DEBUG << std::format("Difference = {0}", FVectorToString(difference));
	auto dot{ uobject->Dot_VectorVector(direction, difference) };
	PLOG_DEBUG << std::format("Dot = {0}", dot);
	auto angle{ acos(dot / (uobject->VSize(difference) * uobject->VSize(direction))) };
	PLOG_DEBUG << std::format("Angle = {0}", angle);
	if (OUT_dot)
		*OUT_dot = dot;
	if (OUT_angle)
		*OUT_angle = angle;
	return dot > 0;
};

std::vector<FVector> GetLocationOfLineIntersectionWithCircleXY(const FVector& lineStart, const FVector& lineEnd,
	const FVector& circleOrigin, const float circleRadii, double* OUT_a, double* OUT_b , double* OUT_c,
	double* OUT_discriminant, double* OUT_t1, double* OUT_t2)
{
	auto uobject = UObject::GObjObjects()->Data[1];
	std::vector<FVector> intersections{};
	auto a{ pow(lineEnd.X - lineStart.X, 2) + pow(lineEnd.Y - lineStart.Y, 2) };
	auto b{ (2 * (lineEnd.X - lineStart.X) * (lineStart.X - circleOrigin.X)) + (2 * (lineEnd.Y - lineStart.Y) * (lineStart.Y - circleOrigin.Y)) };
	auto c{ pow(lineStart.X - circleOrigin.X, 2) + pow(lineStart.Y - circleOrigin.Y, 2) - pow(circleRadii, 2) };
	if (OUT_a)
		*OUT_a = a;
	if (OUT_b)
		*OUT_b = a;
	if (OUT_c)
		*OUT_c = c;
	PLOG_DEBUG << std::format("a = {0}\tb = {1}\tc = {2}", a, b, c);
	auto discriminant{ pow(b, 2) - (4 * a * c) };
	if (OUT_discriminant)
		*OUT_discriminant = discriminant;
	PLOG_DEBUG << std::format("discriminant = {0}", discriminant);
	static const float TINY_NUMBER{ 1E-5 };
	if (discriminant < 0)
	{
		return intersections;
	}
	else if (discriminant > 0 && discriminant < TINY_NUMBER)
	{
		discriminant = 0;
	}

	auto t1{ (-b - sqrt(discriminant)) / (2 * a) };
	auto t2{ (-b + sqrt(discriminant)) / (2 * a) };

	PLOG_DEBUG << std::format("t1 = {0}\tt2={1}", t1, t2);

	if (t1 >= 0 && t1 <= 1)
	{
		auto intersectionLocation = uobject->Add_VectorVector(lineStart, uobject->Multiply_VectorFloat(uobject->Subtract_VectorVector(lineEnd, lineStart), t1));
		intersections.push_back(intersectionLocation);
	}

	if (discriminant == 0)
		return intersections;

	if (t2 >= 0 && t2 <= 1)
	{
		auto intersectionLocation = uobject->Add_VectorVector(lineStart, uobject->Multiply_VectorFloat(uobject->Subtract_VectorVector(lineEnd, lineStart), t2));
		intersections.push_back(intersectionLocation);
	}

	return intersections;
};
