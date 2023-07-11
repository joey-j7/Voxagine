#include "pch.h"
#include "EnemyBullet.h"

#include <Core/ECS/Components/Transform.h>
#include <Core/Resources/Formats/VoxModel.h>
#include <Core/ECS/World.h>
#include <Core/Application.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/Math.h"

#include "Humanoids/Humanoid.h"
#include "Humanoids/Players/Player.h"

#include <External/rttr/registration.h>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"


RTTR_REGISTRATION
{
	rttr::registration::class_<EnemyBullet>("Enemy Bullet")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Bullet Model", &EnemyBullet::m_modelFile)(RTTR_PUBLIC)
	.property("Damage", &EnemyBullet::m_fDamage)(RTTR_PUBLIC)
	.property("Speed", &EnemyBullet::m_fSpeed)(RTTR_PUBLIC)
	.property("Life Time", &EnemyBullet::m_fLifeTime)(RTTR_PUBLIC)
	.property("Bullet Explosion Range", &EnemyBullet::m_fBulletExplosionRange)(RTTR_PUBLIC);
}

EnemyBullet::EnemyBullet(World* world) : 
	Entity(world) 
{
	SetName("Enemy Bullet");
}

void EnemyBullet::Start() 
{
	Entity::Start();

	if (m_direction == Vector3(0))
		m_direction = GetTransform()->GetForward();

	// BoxCollider
	BoxCollider* boxCollider = GetComponent<BoxCollider>();
	if (boxCollider == nullptr)
		boxCollider = AddComponent<BoxCollider>();
	boxCollider->SetTrigger(true);

	// VoxRenderer
	VoxRenderer* voxRenderer = GetComponent<VoxRenderer>();
	if (voxRenderer == nullptr)
		voxRenderer = AddComponent<VoxRenderer>();

	if (!m_modelFile.empty())
	{
		VoxModel* bulletModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox(m_modelFile);
		if (bulletModel != nullptr)
			voxRenderer->SetModel(bulletModel);
	}
	boxCollider->SetBoxSize(voxRenderer->GetFrame());

	SetDestructible(true);
}

void EnemyBullet::FixedTick(const GameTimer& gameTimer) {
	Entity::FixedTick(gameTimer);
	float fDeltaTime = static_cast<float>(gameTimer.GetElapsedSeconds());

	if(IsEnabled()) {
		//Gets the new position of the bullet
		Vector3 velocity = m_direction * m_fSpeed * fDeltaTime;
		GetTransform()->Translate(velocity);

		//Check whether the bullet can be destroyed
		m_fLifeTime -= fDeltaTime;
		if (m_fLifeTime <= 0.f)
			Destroy();
	}
}

void EnemyBullet::OnCollisionEnter(Collider* pCollider, const Manifold& manifold) {
	if (pCollider->GetOwner()->HasTag("Player")) {
		if (const auto humanoid = dynamic_cast<Player*>(pCollider->GetOwner())) {

			// Deal damage to this player
			Vector3 impactNormal = humanoid->GetTransform()->GetPosition() - GetTransform()->GetPosition();
			impactNormal = glm::normalize(impactNormal);
			humanoid->Damage(m_fDamage, impactNormal);
		}
		Destroy();
	}
}

void EnemyBullet::OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled)
{
	for (uint32_t i = 0; i < uiSize; ++i)
	{
		if (voxels[i] && voxels[i]->Active)
		{
			GetWorld()->ApplySphericalDestruction(GetTransform()->GetPosition(), m_fBulletExplosionRange, 50, 50, true);
			Destroy();
			break;
		}
	}
	isHandled = true;
}