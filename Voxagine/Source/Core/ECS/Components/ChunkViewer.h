#pragma once
#include "Core/ECS/Component.h"
#include <rttr/type>

class ChunkViewer : public Component
{
public:
	ChunkViewer(Entity* pOwner);

private:

	RTTR_ENABLE(Component)
};