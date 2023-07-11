#include "pch.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingObstacle.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<pathfinding::PathfindingObstacle>("PathfindingObstacle")
	.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Box size", &pathfinding::PathfindingObstacle::m_halfBoxSize) (RTTR_PUBLIC)
	.property("Discomfort", &pathfinding::PathfindingObstacle::m_fDiscomfort) (RTTR_PUBLIC)
	.property("Is Inmovable", &pathfinding::PathfindingObstacle::getIsInmovable, &pathfinding::PathfindingObstacle::setIsInmovable) (RTTR_PUBLIC);
}

namespace pathfinding
{
	const float PathfindingObstacle::g_INFINTE = std::numeric_limits<float>::max();

	PathfindingObstacle::PathfindingObstacle(Entity* pOwner) :
		Component(pOwner),
		m_pGrid(nullptr),
		m_halfBoxSize(1),
		m_fDiscomfort(1)
	{}

	PathfindingObstacle::~PathfindingObstacle()
	{
		if (m_pGrid != nullptr)
			m_pGrid->removeObstacle(*this);
	}

	void PathfindingObstacle::Start()
	{
		m_pGrid = dynamic_cast<ChunkGrid*>(GetWorld()->FindEntity("PathfindingGrid"));
		if (m_pGrid != nullptr)
			m_pGrid->addObstacle(*this);
	}

	bool PathfindingObstacle::getIsInmovable() const
	{
		return m_fDiscomfort == g_INFINTE;
	}

	void PathfindingObstacle::setIsInmovable(bool inmovable)
	{
		if (inmovable)
			m_fDiscomfort = g_INFINTE;
		else
			m_fDiscomfort = 1;
	}
}