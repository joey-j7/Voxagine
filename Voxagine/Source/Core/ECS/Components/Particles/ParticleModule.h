#pragma once

class ParticlePool;
class ParticleSystem;
class ParticleModule
{
public:
	ParticleModule(ParticleSystem* pSystem) { m_pSystem = pSystem; }
	virtual ~ParticleModule() {}

	virtual void Tick(float fDeltaTime, ParticlePool& particleData) = 0;

protected:
	ParticleSystem* m_pSystem = nullptr;
};