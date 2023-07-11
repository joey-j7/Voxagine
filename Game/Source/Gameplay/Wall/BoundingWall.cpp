#include "BoundingWall.h"

#include <External/rttr/registration>

#include "Core/ECS/World.h"

#include "Core/ECS/Components/Transform.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"

#include "Core/ECS/Systems/Physics/Box.h"

#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<BoundingWall>("Bounding Wall")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Is End Wall", &BoundingWall::bEndWall)(RTTR_PUBLIC, RTTR_TOOLTIP("Should be triggered/closed sooner"));
}

BoundingWall::BoundingWall(World* pWorld) : Entity(pWorld)
{
}

void BoundingWall::Awake()
{
	Entity::Awake();

	const auto Name = "BoundingWall " + std::to_string(GetId());
	SetName( Name );

	m_pCollider = GetComponent<BoxCollider>();
	if (!m_pCollider)
		m_pCollider = AddComponent<BoxCollider>();

	m_pCollider->SetLayer(CL_OBSTACLES);

	m_pParticleSystem = GetComponent<ParticleSystem>();
	if(!m_pParticleSystem)
		m_pParticleSystem = AddComponent<ParticleSystem>();

	SetPersistent(true);
}

void BoundingWall::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);
}

void BoundingWall::OnEnabled()
{
	if (m_bActivated)
		return;

	m_bActivated = true;
	m_pParticleSystem->SetEnabled(true);
	m_pParticleSystem->Play();
}

void BoundingWall::OnDisabled()
{
	if (!m_bActivated)
		return;

	m_bActivated = false;
	m_pParticleSystem->SetEnabled(false);
	m_pParticleSystem->Stop();
}
