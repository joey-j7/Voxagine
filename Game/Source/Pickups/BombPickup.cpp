#include "BombPickup.h"

#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>

#include "Humanoids/Players/Player.h"
#include "Weapons/Weapon.h"

#include "Humanoids/Enemies/Monster.h"
#include "Humanoids/Players/Player.h"

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<BombPickup>("BombPickup")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}


BombPickup::BombPickup(World* world) : GenericPickup(world)
{
	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Projectiles/Bomb.vox");
	PhysicsBody* pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();
	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	pRenderer->SetModel(pModel);
	pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);

	m_OnCollideCallback = std::bind(&BombPickup::OnBombPickedUp, this, std::placeholders::_1);

	SetName("BombPickup");
}

void BombPickup::OnBombPickedUp(Collider* pCollider)
{
	if (pCollider->GetOwner()->HasTag("Player"))
	{
		// Destroy the ammo entity
		Destroy();
	}
}