#include "Monster.h"

#include <External/rttr/registration.h>
#include <Core/MetaData/PropertyTypeMetaData.h>
#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include <Core/Resources/Formats/VoxModel.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/VoxAnimator.h>
// #include <Core/ECS/Components/Particles/ParticleSystem.h>
// #include <Core/ECS/Components/Particles/Emitters/VoxFrameEmitter.h>
// #include <Core/ECS/Components/Particles/Modules/CollisionModule.h>
// #include <Core/ECS/Components/Particles/Modules/AttractorModule.h>
#include "Core/ECS/Components/AudioSource.h"

#include "Humanoids/States/Mon_IdleState.h"
#include "Humanoids/States/Mon_MoveState.h"
#include "Humanoids/States/Mon_MeleeAttackState.h"
#include "Humanoids/States/Mon_RangeAttackState.h"
#include "Weapons/EnemyBullet.h"

#include "AI/FiniteStateMachine.h"
#include "UI/HighScoreUI.h"
#include "General/FlashBehavior.h"
#include "Humanoids/ParticleCorpse.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Monster>("Monster")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Moving Animation", &Monster::m_movingAnimation)(RTTR_PUBLIC)
	.property("Moving Animation FPS", &Monster::m_iMovingAnimationFPS)(RTTR_PUBLIC)
	.property("Idle Animation", &Monster::m_idleAnimation)(RTTR_PUBLIC)(RTTR_CATEGORY("Idle"))
	.property("Idle Animation FPS", &Monster::m_iIdleAnimationFPS)(RTTR_PUBLIC)(RTTR_CATEGORY("Idle"))
	.property("Wake Up Range", &Monster::m_fWakeUpRange)(RTTR_PUBLIC)(RTTR_CATEGORY("Idle"))
	.property("Wake Up Time", &Monster::m_fWakeUpTime)(RTTR_PUBLIC)(RTTR_CATEGORY("Idle"))
	.property("Wake Up Animation", &Monster::m_wakeUpAnimation)(RTTR_PUBLIC)(RTTR_CATEGORY("Idle"))
	.property("Wake Up Animation FPS", &Monster::m_iWakeUpAnimationFPS)(RTTR_PUBLIC)(RTTR_CATEGORY("Idle"))
	.property("Attack Cooldown", &Monster::m_fAttackCooldown)(RTTR_PUBLIC)
	.property("Attack Knockback", &Monster::getKnockback, &Monster::setKnockback)(RTTR_PUBLIC)
	.property("Has Melee Attack", &Monster::m_bCanMeleeAttack)(RTTR_PUBLIC)(RTTR_CATEGORY("Melee Attack"))
	.property("Melee Range", &Monster::m_fMeleeRange)(RTTR_PUBLIC)(RTTR_CATEGORY("Melee Attack"))
	.property("Stop Melee Movement Range", &Monster::m_fStopSeekRange)(RTTR_PUBLIC)(RTTR_CATEGORY("Melee Attack"))
	.property("Melee Attack Damage", &Monster::m_fMeleeAttackDamage)(RTTR_PUBLIC)(RTTR_CATEGORY("Melee Attack"))
	.property("Melee Attack Time", &Monster::m_fMeleeAttackTime)(RTTR_PUBLIC)(RTTR_CATEGORY("Melee Attack"))
	.property("Melee Attack Animation", &Monster::m_meleeAttackAnimation)(RTTR_PUBLIC)(RTTR_CATEGORY("Melee Attack"))
	.property("Melee Attack Animation FPS", &Monster::m_iMeleeAttackAnimationFPS)(RTTR_PUBLIC)(RTTR_CATEGORY("Melee Attack"))
	.property("Bullet Damage", &Monster::m_fBulletDamage)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Bullet Speed", &Monster::m_fBulletSpeed)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Bullet Life Time", &Monster::m_fBulletLifeTime)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Bullet Explosion Range", &Monster::m_fBulletExplosionRange)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Bullet Model File", &Monster::m_bulletModelFile)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Bullet Spawn Offset", &Monster::m_bulletSpawnOffset)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Has Range Attack", &Monster::m_bCanRangeAttack)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Shooting Range", &Monster::m_fShootingRange)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))

	.property("Range Attack Anticipation Time", &Monster::m_fRangeAttackDelayTime)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Range Attack Time", &Monster::m_fRangeAttackTime)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Range Attack Animation", &Monster::m_rangeAttackAnimation)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Range Attack Animation FPS", &Monster::m_iRangeAttackAnimationFPS)(RTTR_PUBLIC)(RTTR_CATEGORY("Range Attack"))
	.property("Apply Default Values", &Monster::m_bApplyDefaultValues)(RTTR_PUBLIC);
}

Monster::Monster(World* world) :
	Humanoid(world),
	m_fWakeUpRange(200.f),
	m_fWakeUpTime(0.f),
	m_fAttackCooldown(1.f),
	m_fCooldownTimer(0.f),
	m_idleAnimation(""),
	m_wakeUpAnimation(""),
	m_iWakeUpAnimationFPS(10),
	m_movingAnimation(""),
	// Melee attack
	m_fMeleeAttackDamage(1.f),
	m_pClosestTarget(nullptr),
	m_meleeAttackAnimation(""),
	m_fMeleeRange(50.f),
	m_fMeleeAttackTime(1.f),
	m_bCanMeleeAttack(false),
	m_bIsMeleeAttacking(false),
	// Range attack
	m_fBulletDamage(1.f),
	m_fBulletSpeed(200.f),
	m_fBulletLifeTime(4.f),
	m_fBulletExplosionRange(10.f),
	m_bulletModelFile(""),
	m_bulletSpawnOffset(0),
	m_fShootingRange(200.f),
	m_fRangeAttackDelayTime(0.3f),
	m_fRangeAttackTime(1.f),
	m_bCanRangeAttack(false),
	m_bIsRangeAttacking(false),
	m_rangeAttackAnimation(""),
	m_fStopSeekRange(0),
	m_bApplyDefaultValues(true)
{
	SetName("Monster");
	m_fDamageKnockBack = 10.f;

	AddState({ "Idle", new Mon_IdleState() });
	AddState({ "Moving", new Mon_MoveState()});
	AddState({ "MeleeAttack", new Mon_MeleeAttackState() });
	AddState({ "RangeAttack", new Mon_RangeAttackState() });

	m_pFiniteStateMachine = new FiniteStateMachine<Monster>(this);

	ApplyDefaultValues();
}

void Monster::Awake() 
{
	Humanoid::Awake();

	if (m_bApplyDefaultValues)
		ApplyDefaultValues();

	// Pathfinder
	pathfinding::Pathfinder* pathfinder = GetComponentAll<pathfinding::Pathfinder>();
	if (pathfinder == nullptr)
		 AddComponent<pathfinding::Pathfinder>();
}

void Monster::Start()
{
	Humanoid::Start();

	if (m_bApplyDefaultValues)
		ApplyDefaultValues();

	// Add tag 
	auto& tags = GetTags();
	if (std::find(tags.begin(), tags.end(), "Enemy") == tags.end())
		tags.push_back("Enemy");

	// Physics
	m_pPhysicsBody->SetGravity(false);
	m_pPhysicsBody->SetStepHeight(5);

	// BoxCollider
	m_pBoxCollider->SetTrigger(true);

	// VoxRenderer
	m_pBoxCollider->SetBoxSize(m_pVoxRenderer->GetFrame());

	// Load animations
	std::vector<std::string> animations;
	if (!m_idleAnimation.empty())
		animations.push_back(m_idleAnimation);
	if (!m_wakeUpAnimation.empty())
		animations.push_back(m_wakeUpAnimation);
	if (!m_movingAnimation.empty())
		animations.push_back(m_movingAnimation);
	if (!m_meleeAttackAnimation.empty())
		animations.push_back(m_meleeAttackAnimation);
	if (!m_rangeAttackAnimation.empty())
		animations.push_back(m_rangeAttackAnimation);
	m_pVoxAnimator->SetAnimationFiles(animations);

	m_pBoxCollider->Destroyed += Event<Component*>::Subscriber([this](Component*)
	{
		m_pBoxCollider->Destroyed -= this;
		m_pBoxCollider = nullptr;

	}, this);

	m_pVoxRenderer->FrameChanged += Event<VoxRenderer*>::Subscriber([this](VoxRenderer*) 
	{
		if(m_pBoxCollider) m_pBoxCollider->AutoFit();
		m_pVoxRenderer->FrameChanged -= this;
	}, this);

	// Set state machine
	SetState("Idle");

	m_InitialScale = GetTransform()->GetScale();
	GetTransform()->SetScale(Vector3(0.f));
}

void Monster::Tick(float fDeltaTime)
{
	Humanoid::Tick(fDeltaTime);

	if (m_fHealth > 0.f)
		m_pFiniteStateMachine->Tick(fDeltaTime);
	if (m_pBoxCollider != nullptr)
		m_pBoxCollider->AutoFit();
}

void Monster::FixedTick(const GameTimer& gameTimer) 
{
	const float fDeltaTime = gameTimer.GetElapsedSeconds();
	m_bAliveTimer += fDeltaTime;

	if (m_bAliveTimer < 1.f)
	{
		m_pFlashingComponent->iTimesToFlash = 5;
		m_pFlashingComponent->dFlashDelay = 0.1f;
		m_pFlashingComponent->FlashingColor = VColor(0.5f, 0.5f, 0.5f, 1.f);
		m_pFlashingComponent->StartFlashing();

		GetTransform()->SetScale(m_InitialScale * std::min(1.f, m_bAliveTimer * 2.f));
	}

	Humanoid::FixedTick(gameTimer);
	m_pFiniteStateMachine->FixedTick(gameTimer);

	PhysicsBody* physicsBody = GetComponent<PhysicsBody>();
	if (physicsBody != nullptr)
	{
		m_Velocity = physicsBody->GetVelocity();
		m_Direction = m_Velocity;
		m_Direction = glm::normalize(m_Direction);
	}
}

void Monster::OnCollisionEnter(Collider* collider, const Manifold&)
{
	// Does the humanoid have a player tag
	if (collider->GetOwner()->HasTag("Player"))
	{
		Humanoid* humanoid = dynamic_cast<Humanoid*>(collider->GetOwner());
		OnAttack(humanoid);
	}
}

void Monster::OnCollisionStay(Collider* collider, const Manifold& manifold)
{
	// Does the humanoid have a player tag
	if (collider->GetOwner()->HasTag("Player"))
	{
		Humanoid* humanoid = dynamic_cast<Humanoid*>(collider->GetOwner());
		OnAttack(humanoid);
	}
}

void Monster::ApplyDefaultValues()
{}

void Monster::OnAttack(Humanoid* pHumanoid)
{
	if (!pHumanoid || m_bAliveTimer < 1.f)
		return;

	// Attack
	if (m_bIsMeleeAttacking)
		pHumanoid->Damage(m_fMeleeAttackDamage, GetTransform()->GetForward(), m_fDamageKnockBack);
}

void Monster::Dead()
{
	bool bIsDead = m_bIsDead;

	Humanoid::Dead();

	if (!bIsDead)
	{
		HighScoreUI::IncrementKillCount();
	}
}

void Monster::MeleeAttack(Vector3& velocity)
{
	assert(m_pClosestTarget);
}

void Monster::RangeAttack()
{
	assert(m_pClosestTarget);

	if (m_bAliveTimer < 1.f)
		return;

	// Calculate bullet rotation
	Quaternion rotation;
	Vector3 direction = m_pClosestTarget->GetTransform()->GetPosition() - GetTransform()->GetPosition();
	if (direction != Vector3(0.f, 0.f, 0.f))
	{
		direction.y = 0.f;
		direction = glm::normalize(direction);
		rotation = glm::quatLookAt(direction, Vector3(0, 1, 0));
	}
	GetTransform()->SetRotation(rotation);

	EnemyBullet* bullet = GetWorld()->SpawnEntity<EnemyBullet>(GetTransform()->GetPosition() + m_bulletSpawnOffset, rotation, Vector3(1));
	bullet->m_fDamage = m_fBulletDamage;
	bullet->m_fSpeed = m_fBulletSpeed;
	bullet->m_fLifeTime = m_fBulletLifeTime;
	bullet->m_fBulletExplosionRange = m_fBulletExplosionRange;
	bullet->m_modelFile = m_bulletModelFile;
}
