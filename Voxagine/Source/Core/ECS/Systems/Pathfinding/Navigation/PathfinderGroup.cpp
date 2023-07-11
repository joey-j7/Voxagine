#include "pch.h"
#include "PathfinderGroup.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGoal.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<pathfinding::PathfinderGroup>("PathfinderGroup")
	.property("Path Smoothing", &pathfinding::PathfinderGroup::m_fPathSmoothing)(RTTR_PUBLIC);
}

namespace pathfinding
{
	int PathfinderGroup::s_nextId = 0;

	PathfinderGroup::PathfinderGroup(World * pWorld) :
		Entity(pWorld),
		m_pGrid(nullptr),
		m_id(s_nextId)
	{
		s_nextId++;
	}

	PathfinderGroup::~PathfinderGroup()
	{
		auto entities = GetWorld()->GetEntities();
		for (auto& entity : entities)
		{
			if (entity == this)
				continue;

			Pathfinder* pathfinder = entity->GetComponent<Pathfinder>();
			if (pathfinder != nullptr)
				pathfinder->m_group = nullptr;

			PathfinderGoal* pathfinderGoal = entity->GetComponent<PathfinderGoal>();
			if (pathfinderGoal != nullptr)
				pathfinderGoal->m_group = nullptr;
		}

		if (m_pGrid != nullptr)
			m_pGrid->removeGroup(*this);
	}

	void PathfinderGroup::Awake()
	{
		Entity::Awake();
	}

	void PathfinderGroup::Start()
	{
		Entity::Start();
		m_pGrid = dynamic_cast<ChunkGrid*>(GetWorld()->FindEntity("PathfindingGrid"));
		assert(m_pGrid);
		if (m_pGrid != nullptr)
			m_pGrid->addGroup(*this);

		for (auto& agent : m_agents)
			agent->updatePositionVelocitySize();
	}

	void PathfinderGroup::Tick(float deltaTime)
	{
		Entity::Tick(deltaTime);

		if (m_goals.size() == 0 || m_agents.size() == 0)
			return;
	}

	int PathfinderGroup::getId() const
	{
		return m_id;
	}

	void PathfinderGroup::addAgent(Pathfinder & pathfinder)
	{
		m_agents.push_back(&pathfinder);
	}

	void PathfinderGroup::removeAgent(Pathfinder & pathfinder)
	{
		if (m_agents.size() > 0)
			m_agents.erase(std::remove(m_agents.begin(), m_agents.end(), &pathfinder), m_agents.end());
	}

	void PathfinderGroup::addGoal(PathfinderGoal & goal)
	{
		m_goals.push_back(&goal);
	}

	void PathfinderGroup::removeGoal(PathfinderGoal & goal)
	{
		if (m_goals.size() > 0)
			m_goals.erase(std::remove(m_goals.begin(), m_goals.end(), &goal), m_goals.end());
	}

	bool PathfinderGroup::isGoal(const IVector3 & nodeWorldPos) const
	{
		auto it = std::find_if(m_goals.begin(), m_goals.end(), 
							   [&nodeWorldPos](const PathfinderGoal* goal) 
							   { return goal->getGoalWorldPos() == nodeWorldPos; });

		return it != m_goals.end();
	}

	void PathfinderGroup::getDesiredVeclocityAndHeight(Vector2 & o_velocity, float & o_height, Node** o_node, Pathfinder & pathfinder) const
	{
		getDesiredVeclocityAndHeight(o_velocity, o_height, o_node, pathfinder.getPosition());
	}

	Vector2 PathfinderGroup::getDesiredVeclocity(Pathfinder & pathfinder) const
	{
		return getDesiredVeclocity(pathfinder.getPosition());
	}

	float PathfinderGroup::getDesiredHeight(Pathfinder & pathfinder) const
	{
		return getDesiredHeight(pathfinder.getPosition());
	}
}