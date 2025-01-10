#include <format>
#include "UnitTest.hpp"
#include "Helper.hpp"
#include <plog/Log.h>

bool PerformUnitTest(void)
{
	auto uobject = UObject::GObjObjects()->Data[1];

	// check IsInFieldOfView
	{
		auto location = FVector(0, 0, 0);
		auto position = FVector(1, 2, 0);
		auto direction = FVector(1, 5, 0);
		auto dot{ 0.0f }, angle{ 0.0f };
		auto result{ IsInFieldOfView(location, direction, position, &dot, &angle) };
		const auto expectedResult{ 11.0 };

		PLOG_INFO << "IsInFieldOfView Test (1)";
		PLOG_INFO << std::format("Dot = {0}", dot);
		PLOG_INFO << std::format("Expected = {0}", expectedResult);
	}

	{
		auto location = FVector(100, 100, 100);
		auto position = FVector(124, 2634, 100);
		auto direction = FVector(1234, 4321, 1000);
		auto dot{ 0.0f }, angle{ 0.0f };
		auto result{ IsInFieldOfView(location, direction, position, &dot, &angle) };
		const auto expectedResult{ 11090090 };

		PLOG_INFO << "IsInFieldOfView Test (2)";
		PLOG_INFO << std::format("Dot = {0}", dot);
		PLOG_INFO << std::format("Expected = {0}", expectedResult);
	}

	//https://www.geogebra.org/m/pKcAsWcY
	// check ProjectOnTo
	{
		auto u = FVector(17.5, 15.4, 0);
		auto v = FVector(8, 2, 0);
		auto projection = uobject->ProjectOnTo(u, v);
		const auto expectedResult = FVector(20.1, 5, 0);

		PLOG_INFO << "ProjectOnTo Test (1)";
		PLOG_INFO << std::format("Result:\t{0}", FVectorToString(projection));
		PLOG_INFO << std::format("Expected:\t{0}", FVectorToString(expectedResult));
	}

	{
		auto u = FVector(17.5, 15.4, 0);
		auto v = FVector(8, 2, 0);
		auto projection = uobject->ProjectOnTo(u, v);
		const auto expectedResult = FVector(20.1, 5, 0);

		PLOG_INFO << "ProjectOnTo Test (1)";
		PLOG_INFO << std::format("Result:\t{0}", FVectorToString(projection));
		PLOG_INFO << std::format("Expected:\t{0}", FVectorToString(expectedResult));
	}

	{
		auto u = FVector(16.2, 13.5, 0);
		auto v = FVector(9.2, -2.9, 0);
		auto projection = uobject->ProjectOnTo(u, v);
		const auto expectedResult = FVector(10.8, -3.4, 0);

		PLOG_INFO << "ProjectOnTo Test (2)";
		PLOG_INFO << std::format("Result:\t{0}", FVectorToString(projection));
		PLOG_INFO << std::format("Expected:\t{0}", FVectorToString(expectedResult));
	}

	{
		auto u = FVector(6.2, 9.8, 0);
		auto v = FVector(21.9, 2.9, 0);
		auto projection = uobject->ProjectOnTo(u, v);
		const auto expectedResult = FVector(7.3, 1, 0);

		PLOG_INFO << "ProjectOnTo Test (3)";
		PLOG_INFO << std::format("Result:\t{0}", FVectorToString(projection));
		PLOG_INFO << std::format("Expected:\t{0}", FVectorToString(expectedResult));
	}

	//https://www.nagwa.com/en/explainers/987161873194/#:~:text=The%20discriminant%20%CE%94%20%3D%20%F0%9D%90%B5%20%E2%88%92%204,and%20the%20circle%20are%20disjoint.
	// check GetLocationOfLineIntersectionWithCircleXY
	{
		auto circleOrigin = FVector(-2, 4, 0);
		auto circleRadius = sqrt(5);
		// y = -2x + 4
		auto lineStart = FVector(-10, -2 * (-10) + 4, 0);
		auto lineEnd = FVector(10, -2 * (10) + 4, 0);

		auto intersections = GetLocationOfLineIntersectionWithCircleXY(lineStart, lineEnd, circleOrigin, circleRadius);

		const auto expectedResult1 = FVector(1.0 / 5, 18.0 / 5, 0);
		const auto expectedResult2 = FVector(-1, 6, 0);

		PLOG_INFO << "GetLocationOfLineIntersectionWithCircleXY Test (1)";
		PLOG_INFO << std::format("Intersections:\t{0}", intersections.size());
		for (int i = 0; i < intersections.size(); i++)
		{
			PLOG_INFO << std::format("Intersection {0}:\t{1}", i, FVectorToString(intersections[i]));
		}
		PLOG_INFO << std::format("Expected 1:\t{0}", FVectorToString(expectedResult1));
		PLOG_INFO << std::format("Expected 2:\t{0}", FVectorToString(expectedResult2));
	}

	return true;
}