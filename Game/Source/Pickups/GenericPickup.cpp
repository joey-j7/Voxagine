#include "GenericPickup.h"
#include <Core/ECS/World.h>

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<GenericPickup>("GenericPickup").constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

GenericPickup::GenericPickup(World* pWorld) : Entity(pWorld) { }

void GenericPickup::OnCollisionEnter(Collider* pCollider, const Manifold&)
{
	if (m_OnCollideCallback) m_OnCollideCallback(pCollider); 
}
