#include "pch.h"
#include "AttractorModule.h"
#include "Core/ECS/Components/Particles/ParticlePool.h"

void AttractorModule::Tick(float fDeltaTime, ParticlePool& particleData)
{
	uint32_t endId = particleData.GetNumAliveParticles();

	for (uint32_t i = 0; i < endId; ++i)
	{
		if (particleData.GridPosition[i].y > 0.f)
			particleData.Velocity[i] += m_AttractionForce * fDeltaTime;
	}
}