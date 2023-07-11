#pragma once
#include "Core/ECS/Components/Particles/ParticleModule.h"

class ParticlePool;
class BasicTimerModule : public ParticleModule
{
public:
	BasicTimerModule(ParticleSystem* pSystem) : ParticleModule(pSystem) {}

	virtual void Tick(float fDeltaTime, ParticlePool& particleData) override;
};