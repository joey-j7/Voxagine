#pragma once

class ParticleSystem;
class ParticlePool;
class ParticleEmitter
{
public:
	ParticleEmitter(ParticleSystem* pSystem) { m_pSystem = pSystem; }
	virtual ~ParticleEmitter() {}

	virtual void Emit(float fDeltatime, ParticlePool& particleData, uint32_t uiStartId, uint32_t uiEndId) = 0;
	void Tick(float fDeltaTime);

protected:
	ParticleSystem* m_pSystem = nullptr;

private:
	float m_fEmissionCounter = 0.f;
};