#pragma once
#include "Core/Math.h"

class ParticlePool
{
public:
	ParticlePool();
	~ParticlePool();

	void Create(uint32_t uiNumParticles);
	void SpawnParticle(uint32_t uiIndex);
	void DestroyParticle(uint32_t uiIndex);
	void SwapParticle(uint32_t a, uint32_t b);

	uint32_t GetNumParticles() const { return m_uiNumParticles; }
	uint32_t GetNumAliveParticles() const { return m_uiNumParticlesAlive; }

	Vector3* Position = nullptr;
	Vector3* GridPosition = nullptr;
	Vector3* Velocity = nullptr;
	VColor* Color = nullptr;
	float* Timer = nullptr;
	bool* Alive = nullptr;

private:
	uint32_t m_uiNumParticles = 0;
	uint32_t m_uiNumParticlesAlive = 0;
};