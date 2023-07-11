#pragma once

#include "Core/Math.h"

class Collider;
class PhysicsBody;
struct Manifold
{
	Vector3 Normal = Vector3(0.f);
	float Overlap = 0.f;
	PhysicsBody* Body1 = nullptr;
	PhysicsBody* Body2 = nullptr;
	Collider* Collider1 = nullptr;
	Collider* Collider2 = nullptr;
	bool ShouldResolve = false;
};