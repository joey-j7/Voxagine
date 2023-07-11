#include "Bullet.h"
#include <Core/ECS/Components/Transform.h>
#include <Core/Resources/Formats/VoxModel.h>
#include <Core/ECS/World.h>
#include <Core/Application.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include "Core/ECS/Systems/Physics/VoxelGrid.h"

#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "General/Managers/WeaponManager.h"

#include <External/rttr/registration.h>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "Core/Math.h"

#include "Humanoids/Players/Player.h"

#include <string>

#include "Core/ECS/Components/AudioSource.h"
#include "AI/Spawner/Spawner.h"
#include "General/CameraMultiplayer.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Bullet>("Bullet")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)

		.property_readonly("Catch Range", &Bullet::m_fCatchRange) (RTTR_PUBLIC)
		.property_readonly("Auto Catch Range", &Bullet::m_fAutoCatchRange) (RTTR_PUBLIC)
		.property_readonly("Perfect Catch Range", &Bullet::m_fPerfectCatchRange) (RTTR_PUBLIC)
		.property_readonly("Minimum Perfect Catch Speed", &Bullet::m_fMinPerfCatchSpeed) (RTTR_PUBLIC)

		.property_readonly("Current Speed", &Bullet::m_fCurrentSpeed) (RTTR_PUBLIC)
		.property_readonly("Max Speed", &Bullet::m_fMaxSpeed) (RTTR_PUBLIC)

		.property_readonly("Travel Timer", &Bullet::m_fTravelTimer) (RTTR_PUBLIC)
		.property_readonly("Travel Time", &Bullet::m_fTravelTime) (RTTR_PUBLIC)

		.property_readonly("Decay Time", &Bullet::m_fDecayTime) (RTTR_PUBLIC)

		.property_readonly("Life Time", &Bullet::m_fLifeTime) (RTTR_PUBLIC)

		.property_readonly("Camera Shake", &Bullet::m_fCameraShake) (RTTR_PUBLIC)
		.property_readonly("Max Camera Shake", &Bullet::m_fMaxCameraShake) (RTTR_PUBLIC)
	;
}

Bullet::Bullet(World* world) : Base(world) { }

void Bullet::Start() 
{
	Base::Start();

	//The bullet is created dynamically, so most of its properties are set in the game manager
	m_pGameManager = dynamic_cast<GameManager*>(GetWorld()->FindEntity("GameManager"));

	// The direction the bullet will be going in
	m_Direction = GetTransform()->GetForward();
	m_Right = GetTransform()->GetRight();

	m_Position = GetTransform()->GetPosition();

	//This will be the name of this bullet entity once it is spawned in the world
	SetName("Bullet");

	/* Add tag */
	AddTag("Bullet");

	if (!m_pGameManager)
	{
		assert(m_pGameManager && "There should be an gamemanager in the world and Persistent");
		m_pGameManager = new GameManager(GetWorld());
		GetWorld()->AddEntity(m_pGameManager);
	}

	m_pWeaponManager = dynamic_cast<WeaponManager*>(GetWorld()->FindEntity("Weapon Manager"));

	if (!m_pWeaponManager)
	{
		// Note we cant make a new weapon manager because it will not contain all the weapon from the serialized world.
		assert(m_pWeaponManager && "Weapon manager is probably not set to persistent or is not found!");
		m_pWeaponManager = new WeaponManager(GetWorld());
		GetWorld()->AddEntity(m_pWeaponManager);
	}

	const WeaponManager::Type* pType = m_pWeaponManager->GetCurrentType();

	// Set ranges for the bullets
	m_fCatchRange = pType->GetCatchRange();
	m_fAutoCatchRange = pType->GetAutoCatchRange();
	m_fPerfectCatchRange = pType->GetPerfectCatchRange();
	m_fMinPerfCatchSpeed = pType->GetMinPerfCatchSpeed();

	m_fBulletExplosionRange = pType->m_fBulletExplosionRange;
	m_fRotationSpeed = pType->m_fRotationSpeed;

	m_fPathAnimationTime = pType->m_fPathAnimationTime;
	m_fPathAnimationAmplitude = pType->m_fPathAnimationAmplitude;
	m_uiPathAnimationLoopMax = pType->m_uiPathAnimationLoopMax;

	m_fTravelTime = pType->m_fTravelTime;
	m_fDecayTime = pType->m_fDecayTime;

	m_MovementType = pType->m_MovementType;

	m_fMaxSpeed = m_pWeaponManager->m_fBulletSpeed;

	m_fMinimalBulletSpeed = m_pWeaponManager->m_fMinimalBulletSpeed;

	m_fBulletVoxelHeight = m_pWeaponManager->m_fBulletVoxelHeight;

	//PhysicsBody* pBody = AddComponent<PhysicsBody>();

	//Add the box collider component statically
	BoxCollider* pCollider = AddComponent<BoxCollider>();

	if (pCollider)
	{
		pCollider->SetContinuousCollision(true);
	}

	m_pAudioSource = AddComponent<AudioSource>();
	m_pAudioSource->SetLooping(false);

	//Add the renderer component statically
	m_pVoxRender = AddComponent<VoxRenderer>();
	if (!m_pVoxRender)
		m_pVoxRender = GetComponent<VoxRenderer>();

	std::string sModelPath;
	const std::string& m_combo0ModelPath = m_pWeaponManager->GetCurrentType() ? m_pWeaponManager->GetCurrentType()->m_combo0ModelPath : "";

	const std::string& m_combo1ModelPath = m_pWeaponManager->GetCurrentType() ? m_pWeaponManager->GetCurrentType()->m_combo1ModelPath : "";

	const std::string& m_combo2ModelPath = m_pWeaponManager->GetCurrentType() ? m_pWeaponManager->GetCurrentType()->m_combo2ModelPath : "";

	const std::string& m_combo3ModelPath = m_pWeaponManager->GetCurrentType() ? m_pWeaponManager->GetCurrentType()->m_combo3ModelPath : "";


	const int iComboStreak = m_pGameManager->GetComboStreak();

	// If streak is 0 or lower than 2
	if (iComboStreak == 0 || iComboStreak < m_pGameManager->comboThreshold1)
	{
		sModelPath = m_combo0ModelPath;
	}
	// If streak is higher or equal 2 and lower than 4 
	else if (iComboStreak >= m_pGameManager->comboThreshold1 && iComboStreak < m_pGameManager->comboThreshold2)
	{
		sModelPath = m_combo1ModelPath;
		m_fMaxSpeed = m_fMaxSpeed * m_pGameManager->speedComboMultiplier1;
	}
	// If streak is higher or equal 4 or lower than 6 
	else if (iComboStreak >= m_pGameManager->comboThreshold2 && iComboStreak < m_pGameManager->comboThreshold3)
	{
		sModelPath = m_combo2ModelPath;
		m_fMaxSpeed = m_fMaxSpeed * m_pGameManager->speedComboMultiplier2;
	}
	// Or is the streak higher than 6
	else if (iComboStreak >= m_pGameManager->comboThreshold3)
	{
		sModelPath = m_combo3ModelPath;
		m_fMaxSpeed = m_fMaxSpeed * m_pGameManager->speedComboMultiplier3;
	}

	if(sModelPath.empty())
	{
		sModelPath = m_combo0ModelPath;
		assert(!sModelPath.empty() && "ModelPath can't be empty");
	}

	if (!sModelPath.empty())
	{
		//Load the model file of the bullet into the game
		VoxModel* pBulletModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox(sModelPath);
		std::string m_sModelPath = sModelPath;
		//Render the bullet model that was loaded
		m_pVoxRender->SetModel(pBulletModel);
	}
	// pRenderer->SetAxisRounded(true);

	//
	m_pVoxRender->SetRotationAngleLimit(0);

	//pBody->SetGravity(false);

	//Set the collider box size to the size of the rendered model
	pCollider->SetBoxSize(m_pVoxRender->GetFrame());

	const Vector3 boxSize = pCollider->GetBoxSize();
	pCollider->SetBoxSize(Vector3(boxSize.x, m_fBulletVoxelHeight, boxSize.z));

	pCollider->SetTrigger(true);

	m_fCurrentSpeed = m_fMaxSpeed;
}

void Bullet::FixedTick(const GameTimer& gameTimer) 
{
	Base::FixedTick(gameTimer);
	
	const float fDeltaTime = static_cast<float>(gameTimer.GetElapsedSeconds());
	m_fTravelTimer += fDeltaTime;

	if (m_bShouldDestroy)
	{
		if (m_fDestroyScaleTimer > 0.f)
		{
			m_fDestroyScaleTimer = std::max(m_fDestroyScaleTimer - (float)gameTimer.GetElapsedSeconds(), 0.f);
			float fNormTime = m_fDestroyScaleTimer / m_fDestroyScaleTime;

			GetTransform()->SetScale(m_ScaleAtDestroy * fNormTime);

			if (m_pCurrentPlayerReceiver)
			{
				GetTransform()->SetPosition(
					glm::mix(
						m_pCurrentPlayerReceiver->GetTransform()->GetPosition(),
						m_PositionAtDestroy,
						fNormTime
					)
				);
			}
		}
		else if (!m_pAudioSource->IsPlaying())
			Destroy();
		else
			return;
	}

	if(bIsReturning)
		GetTransform()->Rotate(Vector3(0.f, m_fMaxSpeed * fDeltaTime * m_fRotationSpeed, 0.f));

	//Makes sure that the component is enabled (I guess. Verify this)
	if(IsEnabled() && !bIsReturning && !m_bShouldDestroy)
	{
		// Decay speed after travel time
		if (m_fTravelTimer >= m_fTravelTime && m_fCurrentSpeed > 0.f)
		{
			//Decrease the bullets speed until it stops
			m_fCurrentSpeed -= m_fMaxSpeed / m_fDecayTime * fDeltaTime;
		}

		if (m_fCurrentSpeed > 0.f)
		{
			m_Velocity = Vector3(0.f);

			switch (m_MovementType)
			{
			case WeaponManager::Type::MT_SINE:
				if (m_bAnimationActivated)
				{
					if (m_uiPathAnimationLoopMax == 0 || m_uiPathAnimationLoopCount < m_uiPathAnimationLoopMax)
					{
						m_Velocity = m_Direction * m_fCurrentSpeed * fDeltaTime;

						m_fPathAnimationTimer += fDeltaTime;

						if (m_fPathAnimationTimer >= m_fPathAnimationTime)
						{
							m_fPathAnimationTimer = std::fmodf(m_fPathAnimationTimer + fDeltaTime, m_fPathAnimationTime);
							m_uiPathAnimationLoopCount++;
						}

						m_AnimationOffset = m_Right * std::sinf(-PI + m_fPathAnimationTimer / m_fPathAnimationTime * PI * 2.f);

						break;
					}
				}

			case WeaponManager::Type::MT_STRAIGHT:
			default:
				m_Velocity = m_Direction * m_fCurrentSpeed * fDeltaTime;
				GetTransform()->Rotate(Vector3(0.f, m_fCurrentSpeed * fDeltaTime * m_fRotationSpeed, 0.f));

				break;
			};
		
			m_Position += m_Velocity;

			//Set the new position of the bullet
			GetTransform()->SetPosition(m_Position + m_AnimationOffset * m_fPathAnimationAmplitude);
		}

		//Increase how long the bullet has been in existence since spawning
		m_fLifeTime += fDeltaTime;

		//Check whether the bullet can be destroyed
		if (m_fLifeTime > m_fDestroyTimer && m_bCanBeDestroyed && m_bTimedDestruction) 
		{	
			MarkAsDestroyed();
		}

		//Update escape procedure when not about to get destroyed 
		if (!m_bShouldDestroy && !m_bEscaped)
		{
			if (glm::distance(GetTransform()->GetPosition(), m_EscapePosition) >= std::max(m_fAutoCatchRange, m_fCatchRange) + m_fEscapeRange){
				m_bEscaped = true;
			}
		}
	}
}

void Bullet::OnDrawGizmos(float fDeltaTime)
{
	DebugSphere sphere;

	sphere.m_Center = GetTransform()->GetPosition();
	sphere.m_fRadius = m_fCatchRange;
	sphere.m_Color = VColors::Orange;

	GetWorld()->GetRenderSystem()->GetDebugRenderer().AddSphere(sphere);

	sphere.m_fRadius = m_fPerfectCatchRange;
	sphere.m_Color = VColors::Yellow;

	GetWorld()->GetRenderSystem()->GetDebugRenderer().AddSphere(sphere);
}

void Bullet::OnCollisionEnter(Collider* pCollider, const Manifold& manifold) 
{
	if (m_bShouldDestroy || m_fCurrentSpeed < m_fMinimalBulletSpeed)
		return;

	if (pCollider->GetOwner()->HasTag("Enemy")) 
	{
		if (const auto humanoid = dynamic_cast<Humanoid*>(pCollider->GetOwner())) 
		{
			Vector3 impactNormal = humanoid->GetTransform()->GetPosition() - GetTransform()->GetPosition();
			impactNormal = glm::normalize(impactNormal);

			// Deal damage to this monster
			humanoid->v3ImpactNormal = -impactNormal;
			humanoid->Damage(GetDamageAmount(), impactNormal);
		}

		m_pGameManager->AddComboStreak(m_pGameManager->enemyComboBonus);
		m_pGameManager->AddToOnComboOnCatch(m_pGameManager->enemyComboBonus);

		ApplyCameraShake();
		m_pAudioSource->SetFilePath("Content/SFX/DiscHitEnemy.ogg");
		m_pAudioSource->Play();

		//Destroy();
	}
	else if(pCollider->GetOwner()->HasTag("Portal"))
	{
		auto pEntity = pCollider->GetOwner();
		if(auto spawnerComp = pEntity->GetComponent<Spawner>())
		{
			ApplyCameraShake();

			Vector3 impactNormal = pEntity->GetTransform()->GetPosition() - GetTransform()->GetPosition();
			impactNormal = -glm::normalize(impactNormal);

			spawnerComp->Damage(GetDamageAmount(), impactNormal);
		}
	}
	else if(!pCollider->GetOwner()->HasTag("Player") && !pCollider->IsTrigger()) // are we colliding with the wall?
	{
		if (!pCollider->GetOwner()->IsDestructible())
		{
			m_Direction = -(2.0f * glm::dot(m_Direction, manifold.Normal) * manifold.Normal - m_Direction); // reflect along the collision plane.

			m_pAudioSource->SetFilePath("Content/SFX/DiscHitProp.ogg");
			m_pAudioSource->Play();
		}
		else
		{
			int rand = glm::linearRand(1, 3);
			m_pAudioSource->SetFilePath("Content/SFX/Explosion" + std::to_string(rand) + ".ogg");
			m_pAudioSource->Play();
		}
	}

	/*else if (pCollider->GetOwner()->HasTag("Player"))
	{
		if ((Entity*)m_pCurrentPlayer == pCollider->GetOwner())
			return;

		m_pCurrentPlayer = reinterpret_cast<Player*>(pCollider->GetOwner());

		m_pWeapon = m_pCurrentPlayer->GetComponent<Weapon>();

		if (m_pWeapon)
		{
			m_pWeapon->SetCurrentAmmo(m_pWeapon->GetCurrentAmmo() + 1);
			m_pGameManager->ResetComboTimer();
			Destroy();
		}
	}*/
}

void Bullet::OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled)
{
	if (m_bShouldDestroy)
		return;

	for (uint32_t i = 0; i < uiSize; ++i)
	{
		if (voxels[i] && voxels[i]->Active)
		{
			if (voxels[i]->UserPointer!=currentModelID)
			{
				m_pGameManager->AddComboStreak(m_pGameManager->environmentComboBonus);
				m_pGameManager->AddToOnComboOnCatch(m_pGameManager->environmentComboBonus);
			}
			currentModelID = voxels[i]->UserPointer;
			GetWorld()->ApplySphericalDestruction(GetTransform()->GetPosition(), m_fBulletExplosionRange, m_pGameManager->voxelExplosionRangeMin, m_pGameManager->voxelExplosionRangeMax, true);
			if (m_fCurrentSpeed > m_fMinimalBulletSpeed)
				ApplyCameraShake();
			break;
		}
	}
	isHandled = true;
}

void Bullet::SetCurrentCaster(Player* pPlayer)
{
	m_pCurrentPlayerCaster = pPlayer;
}

Player* Bullet::GetCurrentCaster() const
{
	return m_pCurrentPlayerCaster;
}

void Bullet::SetCurrentReceiver(Player* pPlayer)
{
	m_pCurrentPlayerReceiver = pPlayer;
}

Player* Bullet::GetCurrentReceiver() const
{
	return m_pCurrentPlayerReceiver;
}

void Bullet::MarkAsDestroyed()
{
	m_bShouldDestroy = true;
	m_ScaleAtDestroy = GetTransform()->GetScale();
	m_PositionAtDestroy = GetTransform()->GetPosition();

	// Destroy the bullet
	std::vector<Player*> pPlayers = GetWorld()->FindEntitiesOfType<Player>();
	
	for (Player* pPlayer : pPlayers)
	{
		pPlayer->RemoveSpawnedBullet(this);
	}
}

void Bullet::ApplyCameraShake() const
{
	auto cameras = GetWorld()->FindEntitiesOfType<CameraMultiplayer>();
	for (auto& camera : cameras)
	{
		float cameraShake = camera->GetCameraShake();
		cameraShake = std::min(cameraShake + m_fCameraShake, m_fMaxCameraShake);
		camera->SetCameraShake(cameraShake);
	}
}
