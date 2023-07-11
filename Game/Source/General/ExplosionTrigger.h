#pragma once
#include <Core/ECS/Entity.h>

class AudioSource;

class ExplosionTrigger : public Entity
{
public:
	ExplosionTrigger(World* world);

	void Awake() override;
	void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) override;

	void Tick(float fDeltaTime) override;

	float m_fExplosionRange = 50.f;
	float m_fExplosionForceMin = 30.f;
	float m_fExplosionForceMax = 80.f;

private:
	AudioSource* m_pAudioSource = nullptr;
	bool m_bDestroyed = false;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};