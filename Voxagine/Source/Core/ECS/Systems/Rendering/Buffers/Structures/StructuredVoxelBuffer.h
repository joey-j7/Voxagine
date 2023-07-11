#pragma once

#include "Core/Math.h"

struct StructuredVoxelBuffer
{
	Vector3 Position;
	Vector3 Extents;
	uint32_t MapperID = 0;
	float Distance = 0.0;
};