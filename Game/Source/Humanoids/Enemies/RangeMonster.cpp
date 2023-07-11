#include "pch.h"
#include "RangeMonster.h"

#include <External/rttr/registration.h>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/PathfinderGoal.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/VoxAnimator.h>
#include <Core/ECS/Systems/Physics/PhysicsSystem.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<RangeMonster>("RangeMonster")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Range", &RangeMonster::m_fRange)(RTTR_PUBLIC);
}

RangeMonster::RangeMonster(World* world) :
	Monster(world),
	m_fRange(50.f),
	m_hasLineOfSight(false),
	m_prevLineOfSight(false),
	m_hasLineOfSightLocks(0)
{}

void RangeMonster::Awake()
{
	Monster::Awake();
	SetName("RangeMonster");
}

void RangeMonster::Start()
{
	Monster::Start();

	VoxRenderer* voxrenderer = GetComponent<VoxRenderer>();
	VoxAnimator* voxAnimator = GetComponent<VoxAnimator>();
	BoxCollider* boxCollider = GetComponent<BoxCollider>();

	voxAnimator->SetSpeed(1.2f);
	boxCollider->SetBoxSize(voxrenderer->GetFrame());
}

void RangeMonster::FixedTick(const GameTimer& gameTimer)
{
	Monster::FixedTick(gameTimer);

	if (IsAlive() && InGame()) 
	{
		pathfinding::Pathfinder* pathfinder = GetComponent<pathfinding::Pathfinder>();
		if (pathfinder != nullptr && pathfinder->m_group != nullptr)
		{
			// Stop pathfinding once in range
			pathfinder->m_findPath = !m_hasLineOfSight;

			// Shoot
			if (m_hasLineOfSight)
			{

			}

			// Check line of sight
			if (m_hasLineOfSightLocks == 0)
			{
				m_hasLineOfSight = m_prevLineOfSight;
				m_prevLineOfSight = false;
				Vector3 position = GetTransform()->GetPosition();
				JobQueue* pJobQueue = GetWorld()->GetJobQueue();
				if (pJobQueue)
				{
					// Check all goals
					for (auto& goal : pathfinder->m_group->m_goals)
					{
						// Is in range
						Vector3 goalPos = goal->getGoalWorldPos();
						if (glm::distance(position, goalPos) < m_fRange)
						{
							// Calculate line of sight
							m_hasLineOfSightLocks++;
							pJobQueue->Enqueue<bool>([this, &position, &goalPos]()
							{
								return calculateLineOfSight(position, goalPos);
							}, [this](bool lineOfSightResult)
							{
								if (lineOfSightResult)
									m_prevLineOfSight = lineOfSightResult;
								m_hasLineOfSightLocks--;
							});
						}
					}
				}
			}
		}
	} else 
		SetEnabled(false);
}

bool RangeMonster::calculateLineOfSight(IVector3 worldPos, IVector3 goalPos)
{
	const VoxelGrid* voxelGrid = GetWorld()->GetPhysics()->GetVoxelGrid();

	// Bressenham line algorithm
	IVector3 delta = goalPos - worldPos;
	IVector3 slope = glm::abs(delta) * 2;

	// Get the sign or 0 of the delta
	IVector3 step;
	step.x = ((delta.x < 0) ? -1 : (delta.x > 0) ? 1 : 0);
	step.y = ((delta.y < 0) ? -1 : (delta.y > 0) ? 1 : 0);
	step.z = ((delta.z < 0) ? -1 : (delta.z > 0) ? 1 : 0);

	IVector3 currentPos = worldPos;

	// Find the dominat axis, default is X
	if (slope.y >= std::max(slope.x, slope.z)) // Y dominant
	{
		std::swap(delta.x, delta.y);
		std::swap(slope.x, slope.y);
		std::swap(step.x, step.y);
		std::swap(currentPos.x, currentPos.y);
		std::swap(goalPos.x, goalPos.y);
	} else if (slope.z >= std::max(slope.x, slope.y)) // Z dominant
	{
		std::swap(delta.x, delta.z);
		std::swap(slope.x, slope.z);
		std::swap(step.x, step.z);
		std::swap(currentPos.x, currentPos.z);
		std::swap(goalPos.x, goalPos.z);
	}

	int32_t currentDeltaY = slope.y - slope.x * 2;
	int32_t currentDeltaZ = slope.z - slope.x * 2;
	while (true)
	{
		// Is air?
		const Voxel* voxel = voxelGrid->GetVoxel(currentPos.x, currentPos.y, currentPos.z);
		if (voxel != nullptr && voxel->Active)
			return false;

		if (currentPos.x == goalPos.x) // Goal has been reached
			return true;

		// Move to next node
		if (currentDeltaY >= 0)
		{
			currentPos.y += step.y;
			currentDeltaY -= slope.x;
		}
		if (currentDeltaZ >= 0)
		{
			currentPos.z += step.z;
			currentDeltaZ -= slope.x;
		}

		currentPos.x += step.x;
		currentDeltaY += slope.y;
		currentDeltaZ += slope.z;
	}

	return false;
}
