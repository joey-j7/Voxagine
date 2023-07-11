#include "pch.h"
#include "Pathfinder.h"

#include "External/rttr/registration.h"
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/ECS/Entity.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/ECS/Components/PhysicsBody.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunk.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGroup.h"
#include "External/optick/optick.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<pathfinding::Pathfinder>("Pathfinder")
	.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Pathfinder Group", &pathfinding::Pathfinder::m_group)(RTTR_PUBLIC)
	.property("Min Velocity", &pathfinding::Pathfinder::m_fMinVelocity)(RTTR_PUBLIC)
	.property("Max Velocity", &pathfinding::Pathfinder::m_fMaxVelocity)(RTTR_PUBLIC)
	.property("Can Move Diagonal", &pathfinding::Pathfinder::m_bCanMoveDiagonal)(RTTR_PUBLIC)
	.property("Find Path", &pathfinding::Pathfinder::m_findPath)(RTTR_PUBLIC)
	.property("Cohesion", &pathfinding::Pathfinder::m_bCohesion)(RTTR_PUBLIC)
	.property("Apply Velocity", &pathfinding::Pathfinder::m_applyVelocity)(RTTR_PUBLIC);
}

namespace pathfinding
{
	Pathfinder::Pathfinder(Entity * pOwner) :
		BehaviorScript(pOwner),
		m_group(nullptr),
		m_findPath(true),
		m_bCanMoveDiagonal(true),
		m_fMinVelocity(170.f),
		m_fMaxVelocity(170.f),
		m_desiredVelocity(0),
		m_flockVelocityX(0),
		m_flockVelocityY(0),
		m_applyVelocity(true),
		m_applyHeight(true),
		m_bIsOnGrid(false),
		m_bCohesion(false),
		m_bClampVelocity(true)
	{}

	Pathfinder::~Pathfinder()
	{
		if (m_group != nullptr)
			m_group->removeAgent(*this);
	}

	void Pathfinder::Start()
	{
		BehaviorScript::Start();

		if (m_group != nullptr)
		{
			m_group->addAgent(*this);
			if (m_group->IsInitialized())
				updatePositionVelocitySize();
		}
	}

	void Pathfinder::FixedTick(const GameTimer& time)
	{
		BehaviorScript::FixedTick(time);

		if (m_findPath && m_group != nullptr && m_group->m_pGrid != nullptr)
		{
			// Agent properties
			Vector3 desiredVelocity;
			Vector3 position = getPosition();
			
			// Get grid properties
			Vector2 gridVelocity = Vector2(0.f);
			float gridHeight = 0.f;
			Node* gridCurrentNode = nullptr;
			m_group->getDesiredVeclocityAndHeight(gridVelocity, gridHeight, &gridCurrentNode, *this);

			if (gridCurrentNode == nullptr)
			{
				m_bIsOnGrid = false;
				return;
			}
			m_bIsOnGrid = true;
			desiredVelocity = Vector3(gridVelocity.x, 0, gridVelocity.y);
			desiredVelocity += Vector3(m_flockVelocityX.load(), 0, m_flockVelocityY.load());

			/*calculatePath();
			if (path.size() > 0)
			{
				float distance = glm::length(Vector3(path[0]) - getPosition());
				if (distance < 8.f)
				{
					path.erase(path.begin());
					if (path.size() == 0)
						return;
					distance = glm::length(Vector3(path[0]) - getPosition());
				}
				
				if (distance < m_group->m_fMinVelocity)
					desiredVelocity = glm::normalize(Vector3(path[0]) - getPosition()) * distance;
				else
					desiredVelocity = glm::normalize(Vector3(path[0]) - getPosition()) * m_group->m_fMinVelocity;
			}*/

			if (!m_bCanMoveDiagonal)
			{
				if (std::abs(desiredVelocity.x) > std::abs(desiredVelocity.z)) desiredVelocity.z = 0;
				else desiredVelocity.x = 0;
			}

			PhysicsBody* physicsBody = GetOwner()->GetComponentAll<PhysicsBody>();
			if (physicsBody != nullptr)
				desiredVelocity = glm::mix(physicsBody->GetVelocity(), desiredVelocity, m_group->m_fPathSmoothing);

			// Clamp velocity
			if (m_bClampVelocity && !m_applyVelocity)
			{
				if (physicsBody != nullptr)
					desiredVelocity = physicsBody->GetVelocity();
			}

			// Clamp velocity to collision
			{
				// North
				bool canMove = gridCurrentNode->m_connections[0].getConnected() && 
							   gridCurrentNode->m_connections[0].m_fHeightGradient <= m_group->m_fMaxHeightGradient;
				if (!canMove && desiredVelocity.z > 0)
					desiredVelocity.z = 0;

				// South
				canMove = gridCurrentNode->m_connections[2].getConnected() && 
					      gridCurrentNode->m_connections[2].m_fHeightGradient <= m_group->m_fMaxHeightGradient;
				if (!canMove && desiredVelocity.z < 0)
					desiredVelocity.z = 0;

				// East
				canMove = gridCurrentNode->m_connections[1].getConnected() && 
					      gridCurrentNode->m_connections[1].m_fHeightGradient <= m_group->m_fMaxHeightGradient;
				if (!canMove && desiredVelocity.x > 0)
					desiredVelocity.x = 0;

				// West
				canMove = gridCurrentNode->m_connections[3].getConnected() && 
					      gridCurrentNode->m_connections[3].m_fHeightGradient <= m_group->m_fMaxHeightGradient;
				if (!canMove && desiredVelocity.x < 0)
					desiredVelocity.x = 0;
			}

			// Clamp velocity
			if (m_bClampVelocity && !m_applyVelocity)
			{
				physicsBody->SetVelocity(desiredVelocity);
			}

			// Set velocity
			if (gridVelocity != Vector2(0) && desiredVelocity != Vector3(0))
			{
				if (glm::length2(desiredVelocity) < m_fMinVelocity * m_fMinVelocity) 
					desiredVelocity = glm::normalize(desiredVelocity) * m_fMinVelocity;
				if (glm::length2(desiredVelocity) > m_fMaxVelocity * m_fMaxVelocity) 
					desiredVelocity = glm::normalize(desiredVelocity) * m_fMaxVelocity;
				m_desiredVelocity = desiredVelocity;
			}

			// Apply Height
			if (m_applyHeight)
			{
				if ((position.y - getHalfBoxSize().y < gridHeight))
					GetTransform()->Translate(Vector3(0, glm::mix(position.y, gridHeight + getHalfBoxSize().y, (float)time.GetElapsedSeconds()  * 4.f * std::abs(position.y - (gridHeight + getHalfBoxSize().y))) - position.y, 0));
				else if (position.y - getHalfBoxSize().y > gridHeight + m_group->m_fMaxHeightGradient)
					GetTransform()->Translate(Vector3(0, (gridHeight + m_group->m_fMaxHeightGradient) - (position.y - getHalfBoxSize().y) - 1, 0));
				else
					GetTransform()->Translate(Vector3(0, glm::mix(position.y, gridHeight + getHalfBoxSize().y, (float)time.GetElapsedSeconds() * 2.f) - position.y, 0));
			}

			// Apply velocity
			if (m_applyVelocity)
			{
				if (physicsBody != nullptr)
				{
					physicsBody->SetVelocity(m_desiredVelocity);
					m_velocity = m_desiredVelocity;
				}
			}
		}
	}

	void Pathfinder::PostTick(float fDeltaTime)
	{
		// Update based on owners physics
		if (m_group != nullptr && m_group->m_pGrid != nullptr)
			updatePositionVelocitySize();
	}

	Vector3 Pathfinder::getPosition()
	{
		return m_position;
	}

	Vector3 Pathfinder::getHalfBoxSize()
	{
		return m_halfBoxSize;
	}

	Vector3 Pathfinder::getVelocity()
	{
		return m_velocity;
	}

	void Pathfinder::calculatePath()
	{
		path.clear();
		IVector3 currentPos = IVector3(glm::round(getPosition()));
		Vector2 desiredVelocity = Vector2(0);
		float height = 0.f;
		Node* node = nullptr;
		m_group->getDesiredVeclocityAndHeight(desiredVelocity, height, &node, currentPos);
		
		int lookAhead = 10;
		for (int i = 0; i < lookAhead; i++)
		{
			if (desiredVelocity == Vector2(0))
				break;

			Vector3 nextPos = Vector3(currentPos) + glm::normalize(Vector3(desiredVelocity.x, 0, desiredVelocity.y)) * (float)sqrt(2 * 16 * 16);
			nextPos /= 16.f;
			currentPos = IVector3(glm::round(nextPos)) * 16;
			if (i != 0) 
				path.push_back(currentPos);

			Vector2 velocity1 = Vector2(0);
			Vector2 velocity2 = Vector2(0);
			Vector2 velocity3 = Vector2(0);
			Vector2 velocity4 = Vector2(0);
			Vector2 velocity5 = Vector2(0);
			velocity1 = m_group->getDesiredVeclocity(currentPos + IVector3(16.f, 0, 0));
			velocity2 = m_group->getDesiredVeclocity(currentPos + IVector3(-16.f, 0, 0));
			velocity3 = m_group->getDesiredVeclocity(currentPos + IVector3(0, 0, 16.f));
			velocity4 = m_group->getDesiredVeclocity(currentPos + IVector3(0, 0, -16.f));
			m_group->getDesiredVeclocityAndHeight(velocity5, height, &node, currentPos);

			desiredVelocity = ((velocity1 + velocity2 + velocity3 + velocity4) / 4.f + velocity5) / 2.f;

			if (node == nullptr || node->m_groupProperties[m_group->getId()].m_bIsGoal)
				break;
		}
	}

	void Pathfinder::updatePositionVelocitySize()
	{
		if (!m_group)
			return;

		assert(GetOwner());
		if (m_group->m_pGrid->m_currentGridJob != ChunkGrid::BUILD_SHARED_FIELD)
		{
			// Get position
			m_position = GetOwner()->GetTransform()->GetPosition();

			// Get velocity
			PhysicsBody* physicsBody = GetOwner()->GetComponentAll<PhysicsBody>();
			if (physicsBody == nullptr)
				m_velocity = Vector3(0.f);
			m_velocity = physicsBody->GetVelocity();

			// Get half box size
			BoxCollider* collider = GetOwner()->GetComponentAll<BoxCollider>();
			if (collider == nullptr)
				m_halfBoxSize = Vector3(0.f);
			m_halfBoxSize =  collider->GetHalfBoxSize();
		}
	}

	bool Pathfinder::IsOnGrid() const
	{
		return m_bIsOnGrid;
	}
}