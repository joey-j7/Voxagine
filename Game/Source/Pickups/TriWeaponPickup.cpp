#include "TriWeaponPickup.h"
#include "Weapons/Weapon.h"

#include "Humanoids/Players/Player.h"

#include "Core/ECS/World.h"
#include "Core/Application.h"

#include <Core/ECS/Components/Collider.h>
#include "Core/ECS/Components/PhysicsBody.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/ECS/Components/VoxRenderer.h"

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<TriWeaponPickup>("TriWeaponPickup")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

TriWeaponPickup::TriWeaponPickup(World* world) : GenericPickup(world)
{
	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/powerup.vox");
	PhysicsBody* pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();
	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	pRenderer->SetModel(pModel);
	pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);

	m_OnCollideCallback = std::bind(&TriWeaponPickup::OnTriWeaponPickedUp, this, std::placeholders::_1);

	SetName("Powerup");
}

void TriWeaponPickup::Start()
{
	GenericPickup::Start();
}


void TriWeaponPickup::OnTriWeaponPickedUp(Collider* pCollider)
{
	if (pCollider->GetOwner()->GetName() == "Player")
	{
		if(Player* player = dynamic_cast<Player*>(pCollider->GetOwner()))
		{
			//player->GetCurrentWeapon()->ResetWeaponAmmo(EWeaponFlags_Triple);
		}
		Destroy();
	}
}