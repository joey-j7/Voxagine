#include "pch.h"
#include "ParticleEmitter.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"
#include "Core/ECS/Components/Particles/ParticlePool.h"

void ParticleEmitter::Tick(float fDeltaTime)
{
	float perParticleRate = 1.f / m_pSystem->GetEmissionRate();

	m_fEmissionCounter += fDeltaTime;
	if (m_fEmissionCounter >= perParticleRate)
	{
		uint32_t maxNewParticles = (uint32_t)(m_fEmissionCounter / perParticleRate);
		m_fEmissionCounter = fmod((float)m_fEmissionCounter, perParticleRate);

		ParticlePool& particleData = m_pSystem->GetParticles();
		uint32_t startId = particleData.GetNumAliveParticles();
		uint32_t endId = std::min(startId + maxNewParticles, particleData.GetNumParticles() - 1);

		Emit(fDeltaTime, particleData, startId, endId);
	}
}