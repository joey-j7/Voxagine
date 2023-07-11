#include "BuildBlock.h"

#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/Transform.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include "Core/ECS/Components/PhysicsBody.h"
#include <Core/ECS/Components/BoxCollider.h>

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<BuildBlock>("BuildBlock")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		)
		.property("Radius", &BuildBlock::m_pOwner)
		.property("Radius", &BuildBlock::m_pBody);
}

BuildBlock::BuildBlock(World* world) : Bullet(world)
{
	// TODO: replace with ammo
	m_pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/build.vox");
	AddComponent<PhysicsBody>();
	AddComponent<BoxCollider>();
	AddComponent<VoxRenderer>();

	SetName("BuildBlock");
}

void BuildBlock::Start()
{
	Bullet::Start();

	m_pBody = GetComponent<PhysicsBody>();
	BoxCollider* pCollider = GetComponent<BoxCollider>();
	VoxRenderer* pRenderer = GetComponent<VoxRenderer>();
	pRenderer->SetModel(m_pModel);
	m_pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);

	m_bCanBeDestroyed = true;

	m_fCurrentSpeed = 100.0f;
	m_fMaxSpeed = 100.0f;
	m_fBulletDamage = 3.0f;

	if (m_pOwner)
	{
		if (m_pBody)
		{
			m_pBody->ApplyImpulse((m_pOwner->GetTransform()->GetForward() + m_pOwner->GetTransform()->GetUp() * 0.5f) * m_fCurrentSpeed);
			
			// Reset it back to zero so that it will cause trouble in the update function
			m_fCurrentSpeed = 0.0f;
		}
	}
}

void BuildBlock::OnCollisionEnter(Collider* pCollider, const Manifold& manifold)
{
	if (pCollider->GetOwner()->GetName() != "Player")
	{
		if (pCollider->GetOwner()->GetName() == "SpawnedEnemy") {
			Bullet::OnCollisionEnter(pCollider, manifold);
		}
		else if (pCollider->GetOwner()->GetName() == "Entity3")
		{ // TODO: Make a generic name
			if (auto physicsBody = GetComponent<PhysicsBody>()) {
				SetStatic(true);
				physicsBody->SetGravity(false);
				physicsBody->SetVelocity(Vector3(0.f));
			}
		}
	}
}