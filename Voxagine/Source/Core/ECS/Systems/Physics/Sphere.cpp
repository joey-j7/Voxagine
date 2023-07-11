#include "pch.h"
#include "Core/ECS/Systems/Physics/Sphere.h"

#include "Core/ECS/Systems/Physics/Box.h"

Sphere::Sphere(const Vector3& center, float radius)
{
	Center = center;
	fRadius = radius;
}

bool Sphere::Intersects(const Sphere& other)
{
	Vector3 diff = Center - other.Center;
	return glm::length(diff) <= fRadius + other.fRadius;
}

bool Sphere::Intersects(const Box& other)
{
	float distSquared = 0.f;
	distSquared += DistPointOutsideSquared(Center.x, other.Min.x, other.Max.x);
	distSquared += DistPointOutsideSquared(Center.y, other.Min.y, other.Max.y);
	distSquared += DistPointOutsideSquared(Center.z, other.Min.z, other.Max.z);

	return distSquared <= fRadius * fRadius;
}

float Sphere::DistPointOutsideSquared(const float fP, const float fMin, const float fMax)
{
	float out = 0.f;

	if (fP < fMin)
	{
		float val = (fMin - fP);
		out += val * val;
	}

	if (fP > fMax)
	{
		float val = (fP - fMax);
		out += val * val;
	}

	return out;
}
