#include "pch.h"
#include "BGMTrigger.h"

#include "Core/ECS/Components/AudioSource.h"

#include <External/rttr/registration.h>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<BGMTrigger>("BGMTrigger")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	;
}

BGMTrigger::BGMTrigger(World * pWorld) :
	Entity(pWorld)
{
	SetName("BGMTrigger");
	SetPersistent(true);
}

void BGMTrigger::Awake()
{
	SetPersistent(true);

	m_pAudioSource = GetComponent<AudioSource>();

	if (!m_pAudioSource)
		m_pAudioSource = AddComponent<AudioSource>();

	m_pAudioSource->SetLooping(true);
	m_pAudioSource->SetBGM(true);

	BoxCollider* pBoxCollider = GetComponent<BoxCollider>();

	if (!pBoxCollider)
		pBoxCollider = AddComponent<BoxCollider>();

	pBoxCollider->SetTrigger(true);
}

void BGMTrigger::Start()
{
	m_pAudioSource->Set3DAudio(false);
	SetPersistent(true);
}

void BGMTrigger::OnCollisionEnter(Collider* pCollider, const Manifold& manifold)
{
	if (pCollider->GetOwner()->GetName() != "Player")
		return;

	m_pAudioSource->Play();
}
