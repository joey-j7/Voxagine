#pragma once
#include "Core/ECS/Components/BehaviorScript.h"
#include "Core/Math.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/ContinuumCrowdsGroup.h"

namespace pathfinding
{
	class Pathfinder : public BehaviorScript
	{
	public:
		bool m_findPath;
		bool m_applyVelocity;
		bool m_applyHeight;
		ContinuumCrowdsGroup* m_group;
		float m_fMinVelocity;
		float m_fMaxVelocity;
		bool m_bCanMoveDiagonal;
		bool m_bCohesion;
		bool m_bClampVelocity;

		std::atomic<float> m_flockVelocityX;
		std::atomic<float> m_flockVelocityY;

	private:
		Vector3 m_position;
		Vector3 m_velocity;
		Vector3 m_halfBoxSize;

		bool m_bIsOnGrid;
		Vector3 m_desiredVelocity;

	public:
		Pathfinder(Entity* pOwner);
		~Pathfinder();
		void Start() override;
		void FixedTick(const GameTimer& time) override;
		void PostTick(float fDeltaTime) override;

		void updatePositionVelocitySize();
		bool IsOnGrid() const;

		virtual Vector3 getPosition();
		virtual Vector3 getHalfBoxSize();
		virtual Vector3 getVelocity();

		RTTR_ENABLE(BehaviorScript)
		RTTR_REGISTRATION_FRIEND

	private:
		std::vector<IVector3> path;
		void calculatePath();
	};
}