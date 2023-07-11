#include "pch.h"
#include "BasicTimerModule.h"
#include "Core/ECS/Components/Particles/ParticlePool.h"

void BasicTimerModule::Tick(float fDeltaTime, ParticlePool& particleData)
{
	uint32_t endId = particleData.GetNumAliveParticles();

	for (uint32_t i = 0; i < endId; ++i)
	{
		particleData.Timer[i] -= fDeltaTime;
		if (particleData.Timer[i] <= 0.f)
		{
			particleData.DestroyParticle(i);
			--endId;
		}
	}
}