#pragma once

#include "Core/Math.h"

class Entity;
struct HitResult
{
	Entity* HitEntity = nullptr;
	Vector3 HitPoint = Vector3(0.f);
};