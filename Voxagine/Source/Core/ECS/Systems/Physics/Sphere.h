#pragma once

#include "Core/Math.h"

class Box;
class Sphere
{
public:
	Sphere() {}
	Sphere(const Vector3& center, float radius);

	Vector3 Center = Vector3(0.f);
	float fRadius = 0.f;

	bool Intersects(const Sphere& other);
	bool Intersects(const Box& other);

private:
	// Calculates the closest squared distance from P to Min or Max if it falls outside otherwise returns zero
	float DistPointOutsideSquared(const float fP, const float fMin, const float fMax);
};