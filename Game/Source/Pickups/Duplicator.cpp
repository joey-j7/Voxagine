#include "Duplicator.h"

#include "Core/Utils/Utils.h"
#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>

#include "AI/Spawner/Spawner.h"

#include "Humanoids/Enemies/Monster.h"

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<Duplicator>("Duplicator")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		)
		.property("Radius", &Duplicator::m_fRadius);
}

Duplicator::Duplicator(World* world) : GenericPickup(world)
{
	VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Pickups/duplicator_pickup.vox");
	PhysicsBody* pBody = AddComponent<PhysicsBody>();
	BoxCollider* pCollider = AddComponent<BoxCollider>();
	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	pRenderer->SetModel(pModel);
	pBody->SetGravity(true);
	pCollider->SetBoxSize(pRenderer->GetFrame());
	pCollider->SetTrigger(true);

	m_OnCollideCallback = std::bind(&Duplicator::OnDuplicatorPicked, this, std::placeholders::_1);

	SetName("Duplicator");
}

void Duplicator::Start()
{
}

void Duplicator::OnDuplicatorPicked(Collider* pCollider)
{
	if (pCollider->GetOwner()->GetName() == "Player")
	{
		const auto monsterEntities = GetWorld()->FindEntities("SpawnedEnemy");

		if(!monsterEntities.empty())
		{
			for (Entity* pMonsterEntity : monsterEntities)
			{
				const float x = pMonsterEntity->GetTransform()->GetPosition().x + m_fRadius * cosf(static_cast<float>(rand() % Utils::FULL_CIRCLE_RADIANS));
				const float z = pMonsterEntity->GetTransform()->GetPosition().z + m_fRadius * sinf(static_cast<float>(rand() % Utils::FULL_CIRCLE_RADIANS));

				// Spawn a new monster around the same position as the current enemy
				auto pEntity = GetWorld()->SpawnEntity<Monster>(Vector3(x, 0.0f, z), Vector3(), Vector3(1.f));
				pEntity->SetName("SpawnedEnemy");

				//if (Monster* pMonster = dynamic_cast<Monster*>(pMonsterEntity)) {
				//	if (pMonster->pFlock) 
				//	{
				//		// Add the entity to the flock
				//		pMonster->pFlock->AddMember(pEntity);
				//		pEntity->pFlock = pMonster->pFlock;
				//	}
				//}
			}
		}
			
		Destroy();
	}
}

