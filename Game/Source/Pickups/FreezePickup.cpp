#include "FreezePickup.h"

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
	rttr::registration::class_<FreezePickup>("FreezePickup")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

FreezePickup::FreezePickup(World* world) : GenericPickup(world)
{
	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Pickups/freeze_pickup.vox");
	PhysicsBody* pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();
	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	pRenderer->SetModel(pModel);
	pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);

	m_OnCollideCallback = std::bind(&FreezePickup::OnFreezePicked, this, std::placeholders::_1);

	SetName("Freeze");
}

void FreezePickup::Start()
{
}

void FreezePickup::OnFreezePicked(Collider* pCollider)
{
	if (pCollider->GetOwner()->GetName() == "Player")
	{
		const auto entities = GetWorld()->FindEntities("SpawnedEnemy");

		if (!entities.empty())
		{
			for (auto& pEntity : entities)
			{
				if (auto enemy = dynamic_cast<Monster*>(pEntity))
				{
					//enemy->SetFreeze(true);
				}
			}
		}

		Destroy();
	}
}

