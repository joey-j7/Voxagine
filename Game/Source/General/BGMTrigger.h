#pragma once
#include <Core/ECS/Entity.h>

class AudioSource;

class BGMTrigger : public Entity
{
public:
	BGMTrigger(World* world);

	void Awake() override;
	void Start() override;
	void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) override;
	
private:
	AudioSource* m_pAudioSource = nullptr;

	bool m_bStartOnPlay = false;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};