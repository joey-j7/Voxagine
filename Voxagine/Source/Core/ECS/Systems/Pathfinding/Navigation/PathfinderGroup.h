#pragma once
#include <vector>
#include <unordered_map>
#include "Core/Math.h"
#include "Core/ECS/Entity.h"

namespace pathfinding
{
	class Pathfinder;
	class PathfinderGoal;
	class ChunkGrid;
	struct Node;
	class PathfinderGroup : public Entity
	{
	public:
		ChunkGrid* m_pGrid;
		std::vector<PathfinderGoal*> m_goals;
		std::vector<Pathfinder*> m_agents;
		float m_fPathSmoothing = 1.f;

	private:
		int m_id;
		static int s_nextId;

	public:
		PathfinderGroup(World* pWorld);
		~PathfinderGroup();
		virtual void Awake() override;
		virtual void Start() override;
		virtual void Tick(float deltaTime) override;

		virtual void updatePaths() = 0;
		virtual void updateAgents(Pathfinder& pathfinder) {};
		int getId() const;

		void addAgent(Pathfinder& pathfinder);
		void removeAgent(Pathfinder& pathfinder);

		void addGoal(PathfinderGoal& goal);
		void removeGoal(PathfinderGoal& goal);
		bool isGoal(const IVector3 & nodeWorldPos) const;

		// Get the desired velocity and height given an agent.
		virtual void getDesiredVeclocityAndHeight(Vector2& o_velocity, float& o_height, Node** o_node, Pathfinder& pathfinder) const;
		virtual Vector2 getDesiredVeclocity(Pathfinder& pathfinder) const;
		virtual float getDesiredHeight(Pathfinder& pathfinder) const;

		// Get the desired velocity and height given an position.
		virtual void getDesiredVeclocityAndHeight(Vector2& o_velocity, float& o_height, Node** o_node, const IVector3& worldPos) const = 0;
		virtual Vector2 getDesiredVeclocity(const IVector3& worldPos) const = 0;
		virtual float getDesiredHeight(const IVector3& worldPos) const = 0;

		RTTR_ENABLE(Entity)
		RTTR_REGISTRATION_FRIEND
	};
}