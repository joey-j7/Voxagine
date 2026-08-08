#pragma once
#include "Core/ECS/Components/BoxCollider.h"

#include "Core/Math.h"

class Box
{
public:
	Box() {};
	Box(BoxCollider* pCollider);

	/* Body here, not in Box.cpp: an inline function must be defined in every
	   translation unit that calls it. */
	inline bool Intersects(const Box& boxB) const
	{
		return (Max.x > boxB.Min.x && Min.x < boxB.Max.x &&
			Max.y > boxB.Min.y && Min.y < boxB.Max.y &&
			Max.z > boxB.Min.z && Min.z < boxB.Max.z);
	}

	bool Intersects(const Box& boxB, Manifold& manifold);

	Vector3 Min = Vector3(0.f);
	Vector3 Max = Vector3(0.f);

	Vector3 GetSize() const { return Max - Min; }
};