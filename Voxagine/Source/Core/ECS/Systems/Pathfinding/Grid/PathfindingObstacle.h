#pragma once
#include "Core/Math.h"
#include "Core/ECS/Component.h"

namespace pathfinding
{
	class ChunkGrid;
	class PathfindingObstacle : public Component
	{
	public:
		static const float g_INFINTE;

		ChunkGrid* m_pGrid;
		Vector3 m_halfBoxSize;
		float m_fDiscomfort;

	public:
		PathfindingObstacle(Entity* pOwner);
		~PathfindingObstacle();
		void Start() override;

		bool getIsInmovable() const;
		void setIsInmovable(bool inmovable);

		RTTR_ENABLE(Component)
		RTTR_REGISTRATION_FRIEND
	};
}