#pragma once
#include "Core/ECS/Components/Particles/ParticleEmitter.h"

class SphereEmitter : public ParticleEmitter
{
public:
	SphereEmitter(ParticleSystem* pSystem) : ParticleEmitter(pSystem) {}

	virtual void Emit(float fDeltatime, ParticlePool& particleData, uint32_t uiStartId, uint32_t uiEndId) override;

	void SetRadius(float fRadius) { m_fRadius = fRadius; }
	float GetRadius() const;

private:
	float m_fRadius = 1.f;
};