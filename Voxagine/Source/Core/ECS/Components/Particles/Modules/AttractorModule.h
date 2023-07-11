#pragma once
#include "Core/ECS/Components/Particles/ParticleModule.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"

class ParticlePool;
class AttractorModule : public ParticleModule
{
public:
	AttractorModule(ParticleSystem* pSystem) : ParticleModule(pSystem) {}

	virtual void Tick(float fDeltaTime, ParticlePool& particleData) override;

	void SetAttractionForce(Vector3 force) { m_AttractionForce = force; }
	Vector3 GetAttractionForce() const { return m_AttractionForce; }

private:
	Vector3 m_AttractionForce = PhysicsSystem::PARTICLE_GRAVITY;
};