#include "pch.h"
#include "SphereEmitter.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"

void SphereEmitter::Emit(float fDeltatime, ParticlePool& particleData, uint32_t uiStartId, uint32_t uiEndId)
{
	Vector3 pos = m_pSystem->GetOwner()->GetTransform()->GetPosition();

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
		particleData.Position[i] = pos + glm::sphericalRand(m_fRadius);

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
		particleData.GridPosition[i] = particleData.Position[i];

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
		particleData.Timer[i] = m_pSystem->GetParticleLifeTime();

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
	{
		Vector3 dir = glm::normalize(pos - particleData.Position[i]);
		particleData.Velocity[i] = dir * m_pSystem->GetParticleStartSpeed();
	}	

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
		particleData.Color[i] = m_pSystem->GetParticleStartColor();

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
		particleData.SpawnParticle(i);
}
