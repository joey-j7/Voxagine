#pragma once
#include "Core/ECS/Components/BoxCollider.h"

#include "Core/Math.h"

class Box
{
public:
	Box() {};
	Box(BoxCollider* pCollider);

	inline bool Intersects(const Box& boxB);
	bool Intersects(const Box& boxB, Manifold& manifold);

	Vector3 Min = Vector3(0.f);
	Vector3 Max = Vector3(0.f);

	Vector3 GetSize() const { return Max - Min; }
};