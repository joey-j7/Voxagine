#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Core/Math.h>

#include "Core/Utils/Utils.h"

TEST(Math, Normalize)
{
	EXPECT_EQ(std::round(glm::length(glm::normalize(Vector3(1, 1, 1)))), 1);
}

/* Test if your vector is producing the right value when used as getting the average */
TEST(Math, VectorAvarage)
{																													
	const Vector3 vector_one = Vector3(2.0f, 4.0f, 8.0f);																				
	const Vector3 vector_two = Vector3(5.0f, 10.0f, 16.0f);																				
	const Vector3 vector_three = Vector3(5.0f, 10.0f, 16.0f);																			
	Vector3 vector_result = vector_one + vector_two + vector_three;													
	Utils::Average(vector_result, 3);

	EXPECT_EQ(std::round(vector_result.x), 4);															
	EXPECT_EQ(std::round(vector_result.y), 8);															
	EXPECT_EQ(std::round(vector_result.z), 13);															
}

/* Test if the c-style array is of the right size when using it for instance in a for-loop */
TEST(Math, LengthArray)
{																													
	int normalarray[] = { 1,2,3,4,5 };
	uint32_t size = 5;																								
	uint32_t arrSize = Utils::Length(normalarray);

	EXPECT_EQ(size, arrSize);																						
};

TEST(Math, ClampValues)
{
	uint32_t value1 = 0;
	uint32_t lo = 1;
	uint32_t hi = 3;

	uint32_t result = Utils::Clamp(value1, lo, hi);

	EXPECT_EQ(lo, result);
}

int main(int argc, char* argv[]) 
{ 
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}