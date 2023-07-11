#include "HealthPack.h"
#include <Core/ECS/Components/Collider.h>

#include "Humanoids/Humanoid.h"

#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<HealthPack>("HealthPack")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		)
		.property("Radius", &HealthPack::m_fRestoredHealth);
}

HealthPack::HealthPack(World* world) : GenericPickup(world)
{
	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Pickups/health_pickup.vox");
	PhysicsBody* pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();
	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	pRenderer->SetModel(pModel);
	pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);

	m_OnCollideCallback = std::bind(&HealthPack::OnHealthPickup, this, std::placeholders::_1);

	SetName("Freeze");
}

void HealthPack::Start()
{
	
}

void HealthPack::OnHealthPickup(Collider* pCollider)
{
	if (pCollider->GetOwner()->GetName() == "Player") {
		if (const auto humanoid = dynamic_cast<Humanoid*>(pCollider->GetOwner())) {
			// deal damage to this monster
			humanoid->AddHealth(m_fRestoredHealth);

			Destroy();
		}
	}
}

