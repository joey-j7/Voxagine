#include "pch.h"
#include "Humanoid.h"

#include "External/rttr/registration.h"

#include "Core/ECS/Entity.h"
#include <Core/ECS/Components/Transform.h>
#include <Core/ECS/World.h>

#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/VoxAnimator.h>

#include <Core/ECS/Components/AudioSource.h>
#include <Core/ECS/Components/AudioPlaylist.h>

#include "Humanoids/ParticleCorpse.h"

// #include <Core/ECS/Components/AudioSource.h>

#include "General/Managers/GameManager.h"
#include "General/FlashBehavior.h"

#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/Resources/Formats/VoxModel.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Humanoid>("Humanoid")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		)
		.property("Damage Offset", &Humanoid::m_DamageOffset)
		.property("Damage Timer", &Humanoid::m_fDamageTimer)
		.property("Damage KnockBack", &Humanoid::m_fDamageKnockBack)

		.property("CoolDown", &Humanoid::m_bCoolDown)
		.property("CoolDown ResetTimer", &Humanoid::m_fCoolDownResetTimer)
		.property("CoolDown Timer", &Humanoid::m_fCoolDownTimer)

		.property("Attack Offset", &Humanoid::m_AttackOffset)
	
		.property_readonly("Is Attacking?", &Humanoid::m_bAttacking)
		.property_readonly("Attack Timer", &Humanoid::m_fAttackTimer)
		.property_readonly("MaxAttack Timer", &Humanoid::m_fMaxAttackTimer)

		.property("Minimal Particle Force", &Humanoid::v3MinForce)(RTTR_PUBLIC)
		.property("Maximal Particle Force", &Humanoid::v3MaxForce)(RTTR_PUBLIC)

		.property("Immortal", &Humanoid::m_bImmortal)

		.property("Health", &Humanoid::GetHealth, &Humanoid::SetHealth)(RTTR_PUBLIC)
		.property("Max Health", &Humanoid::GetMaxHealth, &Humanoid::SetMaxHealth)(RTTR_PUBLIC);

	// FlashBehavior* m_pFlashingComponent = nullptr;
	// GameManager* m_pGameManager = nullptr;
}

Humanoid::Humanoid(World* world) : Base(world)
{
	m_fHealth = m_fMaxHealth = 1.0f;
}

Humanoid::~Humanoid()
{}

void Humanoid::Awake()
{
	Base::Awake();
	AddComponents();
}

void Humanoid::Start() 
{
	Base::Start();
}

void Humanoid::Tick(float fDeltaTime)
{
	Base::Tick(fDeltaTime);

	if (m_fDamageTimer > 0.0f)
		m_fDamageTimer -= fDeltaTime;

	if(m_bCoolDown)
	{
		m_fCoolDownTimer -= GetWorld()->GetDeltaSeconds();
		if(m_fCoolDownTimer <= 0.0f)
		{
			m_bCoolDown = false;
			m_fCoolDownTimer = m_fCoolDownResetTimer;
		}
	}
}

void Humanoid::FixedTick(const GameTimer& gameTimer)
{
	Base::FixedTick(gameTimer);
}

bool Humanoid::InGame() const
{
	return (m_pGameManager && m_pGameManager->IsPlaying());
}

bool Humanoid::Damage(float damage, Vector3 impactNormal, float launchStrength)
{
	if (m_bCoolDown)
		return false;

	if(m_pFlashingComponent)
		m_pFlashingComponent->StartFlashing();

	if (m_fDamageTimer > 0.0f)
		return false;

	if (!m_bImmortal) { // only when we are not immortal	
		m_fHealth -= damage;
		if(m_fHealth < 0.0f)
		{
			m_fHealth = 0.0f;
		}
	}
	m_DamageOffset = impactNormal;
	
	// TODO: see why this should be 1 
	m_DamageOffset = glm::normalize(impactNormal);
	m_DamageOffset.y = 1.f;
	m_DamageOffset *= launchStrength;

	m_fDamageTimer = m_fInvincibilityTimer;
	m_bCoolDown = true;

	m_pPhysicsBody->ApplyImpulse(m_DamageOffset);

	if (!IsAlive())
	{
		Dead();
	}

	return true;
}

void Humanoid::Dead()
{
	if (m_bIsDead)
		return;

	m_bIsDead = true;

	ParticleCorpse* particleEntity = GetWorld()->SpawnEntity<ParticleCorpse>(GetTransform()->GetPosition(), GetTransform()->GetRotation(), GetTransform()->GetScale());
	particleEntity->m_voxFile = m_pVoxRenderer->GetModelFilePath();
	particleEntity->m_MinForce = -v3ImpactNormal * v3MinForce;
	particleEntity->m_MaxForce = -v3ImpactNormal * v3MaxForce;
	particleEntity->m_fArcAngle = 45.f;
	particleEntity->m_SplashDirection = -v3ImpactNormal;

	Destroy();
}

VoxModel* Humanoid::GetAnimation(const std::string& sName) const
{
	auto it = m_mAnimations.find(sName);

	if (it != m_mAnimations.end())
		return it->second;

	return nullptr;
}

void Humanoid::SetAnimation(const std::string& sName)
{
	VoxModel* animation = GetAnimation(sName);
	if (animation != nullptr)
		GetVoxAnimator()->SetAnimModelFilePath(animation->GetRefPath());
}

void Humanoid::AddComponents()
{
	// Physics
	m_pPhysicsBody = GetComponent<PhysicsBody>();
	if (!m_pPhysicsBody)
		m_pPhysicsBody = AddComponent<PhysicsBody>();

	// BoxCollider
	m_pBoxCollider = GetComponent<BoxCollider>();
	if (!m_pBoxCollider)
		m_pBoxCollider = AddComponent<BoxCollider>();

	// VoxRenderer
	m_pVoxRenderer = GetComponent<VoxRenderer>();
	if (!m_pVoxRenderer)
		m_pVoxRenderer = AddComponent<VoxRenderer>();

	// VoxAnimator
	m_pVoxAnimator = GetComponent<VoxAnimator>();
	if (!m_pVoxAnimator)
		m_pVoxAnimator = AddComponent<VoxAnimator>();

	// AudioSource
	m_pAudioSource = GetComponent<AudioSource>();
	if (!m_pAudioSource)
		m_pAudioSource = AddComponent<AudioSource>();

	// AudioPlaylist
	m_pAudioPlaylist = GetComponent<AudioPlaylist>();
	if (!m_pAudioPlaylist)
		m_pAudioPlaylist = AddComponent<AudioPlaylist>();

	// FlashComponent
	m_pFlashingComponent = GetComponent<FlashBehavior>();
	if (!m_pFlashingComponent)
		m_pFlashingComponent = AddComponent<FlashBehavior>();
}
