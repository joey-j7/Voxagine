#include "pch.h"
#include "KillTrigger.h"

#include <External/rttr/registration.h>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "Humanoids/Enemies/Monster.h"
#include "Core/ECS/Components/BoxCollider.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<KillTrigger>("KillTrigger")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	;
}

KillTrigger::KillTrigger(World* pWorld) :
	Entity(pWorld)
{

}

void KillTrigger::Awake()
{
	SetName("KillTrigger");

	BoxCollider* pBoxCollider = GetComponent<BoxCollider>();

	if (!pBoxCollider)
		pBoxCollider = AddComponent<BoxCollider>();

	pBoxCollider->SetTrigger(true);
}

void KillTrigger::OnCollisionEnter(Collider* pCollider, const Manifold& manifold)
{
	if (Monster* pMonster = dynamic_cast<Monster*>(pCollider->GetOwner()))
	{
		Vector3 dir = glm::linearRand(Vector3(1, 0, 0), Vector3(0, 0, 1));
		dir = glm::normalize(dir);
		pMonster->Damage(pMonster->GetMaxHealth(), dir);
	}
}