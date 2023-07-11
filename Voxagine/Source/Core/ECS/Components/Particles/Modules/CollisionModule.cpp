#include "pch.h"
#include "CollisionModule.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Components/Particles/ParticlePool.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"

void CollisionModule::Tick(float fDeltaTime, ParticlePool& particleData)
{
	VoxelGrid* pGrid = m_pSystem->GetWorld()->GetPhysics()->GetVoxelGrid();
	uint32_t endId = particleData.GetNumAliveParticles();

	for (uint32_t i = 0; i < endId; ++i)
	{
		Vector3 prevGridPos = particleData.GridPosition[i];
		Vector3 newGridPos = pGrid->WorldToGrid(particleData.Position[i], true);

		if (particleData.Position[i].y < 1.f)
			particleData.Position[i].y = 1.f;

		// Clamp y to zero to avoid particles being destroyed by to high velocities
		if (newGridPos.y < 0.f)
			newGridPos.y = 0.f;

		if (prevGridPos != newGridPos)
		{
			Voxel* pVoxel = pGrid->GetVoxel(
				static_cast<int>(newGridPos.x),
				static_cast<int>(newGridPos.y),
				static_cast<int>(newGridPos.z)
			);

			Vector3 relPos = prevGridPos - newGridPos;
			if (pVoxel && pVoxel->Active)
			{
				Vector3 normal;
				if (newGridPos.y <= 0.f)
				{
					normal = Vector3(0.f, 1.f, 0.f);
				}
				else
				{
					normal = glm::normalize(relPos);
					if (glm::distance2(prevGridPos, newGridPos) > 1.f)
					{
						normal.x = 0.f;
						normal.z = 0.f;
						normal.y = round(normal.y);
					}
				}
				
				float contactVel = glm::dot(particleData.Velocity[i], normal);

				Vector3 tangentVelocity = particleData.Velocity[i] - normal * contactVel;
				Vector3 tangent = glm::normalize(tangentVelocity);

				float frictionVel = glm::dot(tangentVelocity, tangent);

				if (contactVel < 0)
				{
					/* Apply simple bounce impulse */
					particleData.Velocity[i] += -normal * contactVel * 1.5f;

					/* Apply simple friction impulse */
					particleData.Velocity[i] += -tangent * frictionVel * 0.5f;
				}
			}

			particleData.GridPosition[i] = newGridPos;
		}
	}
}