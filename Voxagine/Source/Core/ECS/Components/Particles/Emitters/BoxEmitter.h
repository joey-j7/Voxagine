#pragma once
#include "Core/ECS/Components/Particles/ParticleEmitter.h"

class BoxEmitter : public ParticleEmitter
{
public:
	BoxEmitter(ParticleSystem* pSystem) : ParticleEmitter(pSystem) {}

	virtual void Emit(float fDeltatime, ParticlePool& particleData, uint32_t uiStartId, uint32_t uiEndId) override;

	void SetBoxSize(Vector3 boxSize);
	Vector3 GetBoxSize() const;

private:
	Vector3 m_boxSizeHalf = Vector3(10, 10, 10);
};