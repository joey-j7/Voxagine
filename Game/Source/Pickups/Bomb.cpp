#include "Bomb.h"

#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/Transform.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Components/PhysicsBody.h"
#include <Core/ECS/Components/BoxCollider.h>

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<Bomb>("Bomb")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		)
		.property("Radius", &Bomb::m_pOwner)
		.property("Radius", &Bomb::m_pBody)
		.property("Radius", &Bomb::m_fBombRadius)
		.property("Radius", &Bomb::m_fBombForce)
		.property("Radius", &Bomb::m_fLife)
		.property_readonly("Radius", &Bomb::m_fDestroyTimer)
		.property("Radius", &Bomb::m_fForce);
}

Bomb::Bomb(World* world) : Entity(world)
{
	// TODO: replace with ammo
	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Projectiles/Bomb.vox");
	m_pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();
	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	pRenderer->SetModel(pModel);
	m_pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);
	pCollider->SetContinuousVoxelCollision(true);

	SetName("Bomb");
}

void Bomb::Start()
{
	Entity::Start();

	if(m_pOwner)
	{
		if (m_pBody) 
		{
			m_pBody->ApplyImpulse((m_pOwner->GetTransform()->GetForward() + m_pOwner->GetTransform()->GetUp()) * m_fForce);
		}
	}
}

void Bomb::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);
	if (IsEnabled()) 
	{
		m_fLife += GetWorld()->GetDeltaSeconds();
		if (m_fLife > m_fDestroyTimer) {
			Destroy();
		}
	}
}


void Bomb::OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled)
{
	for (uint32_t i = 0; i < uiSize; ++i)
	{
		if (voxels[i] && voxels[i]->Active)
		{
			GetWorld()->ApplySphericalDestruction(GetTransform()->GetPosition(), m_fBombRadius, m_fBombForce, m_fBombForce, true);
			Destroy();
			break;
		}
	}

	isHandled = true;
}
