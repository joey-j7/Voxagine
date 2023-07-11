#pragma once
#include "Core/ECS/Entity.h"

class World;
class ParticleCorpse : public Entity
{
public:
	std::string m_voxFile;
	float m_fParticleLifeTime = 5.f;
	Vector3 m_MaxForce = Vector3(0.f);
	Vector3 m_MinForce = Vector3(0.f);
	Vector3 m_SplashDirection = Vector3(0.f);
	float m_fArcAngle = 45.f;

private:
	float m_fTimer = 0.f;
	bool m_bParticlesSpawned = false;
	bool m_bParticlesInitialized = false;

public:
	ParticleCorpse(World* world);

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Tick(float fDeltaTime) override;

	RTTR_ENABLE(Entity)
};