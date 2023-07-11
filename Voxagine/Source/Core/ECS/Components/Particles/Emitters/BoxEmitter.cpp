#include "pch.h"
#include "BoxEmitter.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"

void BoxEmitter::Emit(float fDeltatime, ParticlePool& particleData, uint32_t uiStartId, uint32_t uiEndId)
{
	Vector3 pos = m_pSystem->GetOwner()->GetTransform()->GetPosition();
	Vector3 dir = m_pSystem->GetOwner()->GetTransform()->GetForward();

	Vector3 minBox(pos - m_boxSizeHalf);
	Vector3 maxBox(pos + m_boxSizeHalf);

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
		particleData.Position[i] = glm::linearRand(minBox, maxBox);;

	for (uint32_t i = uiStartId; i < uiEndId; ++i) 
		particleData.GridPosition[i] = particleData.Position[i];

	for (uint32_t i = uiStartId; i < uiEndId; ++i) 
		particleData.Timer[i] = m_pSystem->GetParticleLifeTime();

	for (uint32_t i = uiStartId; i < uiEndId; ++i) 
		particleData.Velocity[i] = dir * m_pSystem->GetParticleStartSpeed();

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
		particleData.Color[i] = m_pSystem->GetParticleStartColor();

	for (uint32_t i = uiStartId; i < uiEndId; ++i)
		particleData.SpawnParticle(i);
}

void BoxEmitter::SetBoxSize(Vector3 boxSize)
{
	m_boxSizeHalf = boxSize * 0.5f;
}

Vector3 BoxEmitter::GetBoxSize() const
{
	return m_boxSizeHalf * 2.f;
}
