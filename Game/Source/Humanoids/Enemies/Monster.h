#pragma once
#include "Humanoids/Humanoid.h"
#include "AI/IFiniteManager.h"
#include "AI/States/FSMState.h"

class VoxAnimator;

namespace pathfinding
{
	class Pathfinder;
}
class Monster : public Humanoid, public IFiniteManager<Monster>
{
public:
	std::string m_idleAnimation;
	int m_iIdleAnimationFPS = 10;
	std::string m_movingAnimation;
	int m_iMovingAnimationFPS = 10;

	float m_fWakeUpRange;
	float m_fWakeUpTime;
	std::string m_wakeUpAnimation;
	int m_iWakeUpAnimationFPS = 10;

	float m_fCooldownTimer;
	float m_fAttackCooldown;
	Entity* m_pClosestTarget;

	float m_fMeleeAttackDamage;
	float m_fMeleeRange;
	float m_fMeleeAttackTime;
	bool m_bCanMeleeAttack;
	bool m_bIsMeleeAttacking;
	std::string m_meleeAttackAnimation;
	int m_iMeleeAttackAnimationFPS = 10;
	float m_fStopSeekRange;

	float m_fBulletDamage;
	float m_fBulletSpeed;
	float m_fBulletLifeTime;
	float m_fBulletExplosionRange;
	std::string m_bulletModelFile;
	Vector3 m_bulletSpawnOffset;
	float m_fShootingRange;
	float m_fRangeAttackDelayTime;
	float m_fRangeAttackTime;
	bool m_bCanRangeAttack;
	bool m_bIsRangeAttacking;
	std::string m_rangeAttackAnimation;
	int m_iRangeAttackAnimationFPS = 10;
	bool m_bIsIdle;

	bool m_bApplyDefaultValues;

protected:
	VoxAnimator* m_animator = nullptr;
	Vector3 m_InitialScale = Vector3(1.f, 1.f, 1.f);

public:
	Monster(World* world);
	virtual ~Monster() = default;

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Tick(float fDeltaTime) override;
	virtual void FixedTick(const GameTimer& gameTimer)  override;

	virtual void OnCollisionEnter(Collider* collider, const Manifold&) override;
	virtual void OnCollisionStay(Collider* collider, const Manifold&) override;

	virtual void ApplyDefaultValues();

	virtual void OnAttack(Humanoid* pHumanoid);

	virtual void Idle() {};
	virtual void Dead() override;

	virtual void MeleeAttack(Vector3& velocity);
	virtual void RangeAttack();

	float getKnockback() const { return m_fDamageKnockBack; }
	void setKnockback(float knockback) { m_fDamageKnockBack = knockback;  }

	RTTR_ENABLE(Humanoid)
	RTTR_REGISTRATION_FRIEND;
};

