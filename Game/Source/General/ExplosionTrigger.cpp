#include "pch.h"
#include "ExplosionTrigger.h"

#include "Core/ECS/World.h"

#include <External/rttr/registration.h>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include <Core/ECS/Components/AudioSource.h>
#include <Core/ECS/Components/BoxCollider.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<ExplosionTrigger>("ExplosionTrigger")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Explosion Range", &ExplosionTrigger::m_fExplosionRange, rttr::registration::public_access) (RTTR_PUBLIC)
		.property("Explosion Force Min", &ExplosionTrigger::m_fExplosionForceMin, rttr::registration::public_access) (RTTR_PUBLIC)
		.property("Explosion Force Max", &ExplosionTrigger::m_fExplosionForceMax, rttr::registration::public_access) (RTTR_PUBLIC)
	;
}

ExplosionTrigger::ExplosionTrigger(World* world) : Entity(world)
{
	SetName("ExplosionTrigger");
}

void ExplosionTrigger::Awake()
{
	m_pAudioSource = GetComponent<AudioSource>();

	if (!m_pAudioSource)
		m_pAudioSource = AddComponent<AudioSource>();

	BoxCollider* pBoxCollider = GetComponent<BoxCollider>();

	if (!pBoxCollider)
		pBoxCollider = AddComponent<BoxCollider>();

	pBoxCollider->SetTrigger(true);
}

void ExplosionTrigger::OnCollisionEnter(Collider* pCollider, const Manifold& manifold)
{
	if (m_bDestroyed)
		return;

	if (!pCollider->GetOwner()->HasTag("Player"))
		return;

	m_bDestroyed = true;

	Entity::OnCollisionEnter(pCollider, manifold);

	if (m_pAudioSource->GetSoundReference())
		m_pAudioSource->Play();

	GetWorld()->ApplySphericalDestruction(GetTransform()->GetPosition(), m_fExplosionRange, m_fExplosionForceMin, m_fExplosionForceMax, true);
}

void ExplosionTrigger::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);

	if (m_bDestroyed && !m_pAudioSource->IsPlaying())
	{
		Destroy();
	}
}

