#include "Projectile.h"

#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/Transform.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/Components/PhysicsBody.h"
#include <Core/ECS/Components/BoxCollider.h>

#include "Core/Math.h"

#include "Humanoids/Players/Player.h"

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<Projectile>("Projectile")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		)
		.property("Owner", &Projectile::m_pOwner)
		.property("Body", &Projectile::m_pBody)
		.property("Detonation Radius", &Projectile::m_fDetonationRadius)
		.property("Detonation Force", &Projectile::m_fDetonationForce)
		.property("Life", &Projectile::m_fLife)
		.property_readonly("Destroy Timer", &Projectile::m_fDestroyTimer);
}


Projectile::Projectile(World* world) : Entity(world)
{
	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Projectiles/Bomb.vox");
	m_pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();
	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	pRenderer->SetModel(pModel);
	m_pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);
	pCollider->SetContinuousVoxelCollision(false);

	SetName("Projectile");

}

void Projectile::Start()
{
	Entity::Start();
	//get the player in the world, dynamic_cast takes the object try to cast to different object, if it fails return a nullptr if it find it return the component
	m_pPlayer = dynamic_cast<Player*>(GetWorld()->FindEntity("Player"));

	//get the position of the aim
	m_Direction = GetTransform()->GetForward();
}


void Projectile::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);

	if (IsEnabled())
	{

		if (m_fSpeed > 0)
		{
			m_fSpeed -= 75.0f * GetWorld()->GetDeltaSeconds();
		}
		
		const Vector3 CurrentPosition = m_Direction * m_fSpeed * GetWorld()->GetDeltaSeconds();
		GetTransform()->Translate(CurrentPosition); 

		m_fLife += GetWorld()->GetDeltaSeconds();
		if (m_fLife > m_fDestroyTimer) {
			Destroy();
		}
		
		//When projectile get to position desapear 
		/*float dist = glm::distance(GetTransform()->GetPosition(), aimPos);
		if (dist < Destroy_dist) {
			Destroy();
		}*/
	}
}

 