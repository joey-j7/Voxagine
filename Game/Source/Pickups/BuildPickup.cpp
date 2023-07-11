#include "BuildPickup.h"

#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>

#include "Humanoids/Players/Player.h"
#include "Weapons/Weapon.h"
#include "Humanoids/Enemies/Monster.h"

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<BuildPickup>("BuildPickup")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

BuildPickup::BuildPickup(World* world) : GenericPickup(world)
{
	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Pickups/build_pickup.vox");
	PhysicsBody* pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();
	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	pRenderer->SetModel(pModel);
	pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);

	m_OnCollideCallback = std::bind(&BuildPickup::OnBuildPickedUp, this, std::placeholders::_1);

	SetName("Build");
}

void BuildPickup::OnBuildPickedUp(Collider* pCollider)
{
	if (pCollider->GetOwner()->GetName() == "Player")
	{
		// Destroy the ammo entity
		Destroy();
	}
}

