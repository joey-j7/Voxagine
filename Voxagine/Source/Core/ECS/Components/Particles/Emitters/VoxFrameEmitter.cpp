#include "pch.h"
#include "VoxFrameEmitter.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"

void VoxFrameEmitter::Emit(float fDeltatime, ParticlePool& particleData, uint32_t uiStartId, uint32_t uiEndId)
{
	if (m_pRenderer == nullptr)
		return;

	if (m_pRenderer->m_BakeData.Positions)
	{
		RenderSystem* pRenderSystem = m_pSystem->GetWorld()->GetRenderSystem();
		const VoxelGrid* pGrid = m_pSystem->GetWorld()->GetVoxelGrid();
		uint32_t* bakePositionData = m_pRenderer->m_BakeData.Positions;
		uint32_t arrSize = m_pRenderer->m_BakeData.Size;
		Vector3 dir = m_pSystem->GetOwner()->GetTransform()->GetForward();

		for (uint32_t i = uiStartId; i < uiEndId; ++i)
		{
			particleData.Position[i] = Vector3(-1000000, -1000000, -1000000);
			particleData.Color[i] = VColor(0.f, 0.f, 0.f, 0.f);
			particleData.GridPosition[i] = particleData.Position[i];
		}

		for (uint32_t i = 0; i < arrSize; ++i)
		{
			if (bakePositionData[i] == UINT_MAX)
				continue;

			if (uiStartId + i < uiEndId)
			{
				particleData.Position[uiStartId + i] = (Vector3)pGrid->GetVoxelPosition(bakePositionData[i]) + pGrid->GetWorldOffset();
				particleData.Color[uiStartId + i] = pRenderSystem->GetVoxel(bakePositionData[i]);
				particleData.GridPosition[uiStartId + i] = particleData.Position[uiStartId + i];
			}
		}

		for (uint32_t i = uiStartId; i < uiEndId; ++i)
			particleData.Timer[i] = m_pSystem->GetParticleLifeTime();

		float minForceLen = glm::length(m_MinForce);
		float maxForceLen = glm::length(m_MaxForce);
		float angle = atan2(m_SplashDirection.x, m_SplashDirection.z);

		for (uint32_t i = uiStartId; i < uiEndId; ++i)
		{
			Vector3 sphereRand = Utils::SphericalRand(1.f, 0.f, PI, 1.f, std::cos(m_fArcAngle));
			Vector3 rotatedRand = glm::normalize(glm::rotate(glm::angleAxis(angle, Vector3(0, 1, 0)), sphereRand));
			Vector3 minForce = (glm::normalize(m_MinForce) + rotatedRand) * minForceLen;
			Vector3 maxForce = (glm::normalize(m_MaxForce) + rotatedRand) * maxForceLen;

			particleData.Velocity[i] = glm::linearRand(minForce, maxForce) * m_pSystem->GetParticleStartSpeed();
		}

		for (uint32_t i = uiStartId; i < uiEndId; ++i)
			particleData.SpawnParticle(i);
	}
}