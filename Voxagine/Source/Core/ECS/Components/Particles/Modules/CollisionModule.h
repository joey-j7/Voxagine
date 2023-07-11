#pragma once
#include "Core/ECS/Components/Particles/ParticleModule.h"

class ParticlePool;
class CollisionModule : public ParticleModule
{
public:
	CollisionModule(ParticleSystem* pSystem) : ParticleModule(pSystem) {}

	virtual void Tick(float fDeltaTime, ParticlePool& particleData) override;
};