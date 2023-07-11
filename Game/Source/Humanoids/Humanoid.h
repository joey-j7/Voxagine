#pragma once

#include "Core/ECS/Entity.h"
#include "Core/Math.h"

#include <string>
#include <unordered_map>

class Weapon;
class FlashBehavior;
class GameManager;
class BoxCollider;
class VoxRenderer;
class VoxAnimator;
class PhysicsBody;

class AudioSource;
class AudioPlaylist;

class VoxModel;

class Humanoid : public Entity {
public:
	bool m_bDidPlayDeathSound = false;

public:
	/*!
	 * @param world
	 */
	Humanoid(World* world);
	virtual ~Humanoid();

	virtual void Reset() {}

	void Awake() override;
	void Start() override;

	virtual void Tick(float fDeltaTime) override;
	virtual void FixedTick(const GameTimer& gameTimer) override;

	bool InGame() const;

	/*!
	 * @brief - Reduces the health
	 * @param damage
	 * @param impactNormal
	 * @param fLaunchStrength
	 */
	virtual bool Damage(float damage, Vector3 impactNormal, float fLaunchStrength = 1.0f);

	virtual void Dead();

	float GetHealth() const { return m_fHealth; }

	/*!
	 * @brief - Set the entity health
	 * @param health
	 */
	void SetHealth(float health) { m_fHealth = health; }

	/*!
	 * @brief - Set max health of the entity
	 * @param maxHealth
	 */
	void SetMaxHealth(float maxHealth) { m_fMaxHealth = maxHealth; }

	/*!
	 * @brief get max health of the entity
	 */
	float GetMaxHealth() const { return m_fMaxHealth; }

	/*!
	 * @brief - To see if the entity is still alive
	 */
	virtual bool IsAlive() const { 
		return !(m_fHealth <= 0.0f); 
	}

	/*!
	 * @brief - Restores the health by n
	 * @param health
	 */
	void AddHealth(float health) { m_fHealth += health; }

	/*!
	 * @brief to see if the entity is attacking
	 */
	bool IsAttacking() const { return m_fAttackTimer > 0.0f; }

	const Vector3& GetVelocity() const { return m_Velocity; }
	const Vector3& GetDirection() const { return m_Direction; }

	AudioPlaylist* GetAudioPlaylist() const { return m_pAudioPlaylist; }
	VoxRenderer* GetVoxRenderer() const { return m_pVoxRenderer; }
	VoxAnimator* GetVoxAnimator() const { return m_pVoxAnimator; }

	VoxModel* GetAnimation(const std::string& sName) const;
	void SetAnimation(const std::string& sName);

	Vector3 v3ImpactNormal = Vector3(0.0f);
	Vector3 v3MinForce = Vector3(5.f, 1.f, 5.f);
	Vector3 v3MaxForce = Vector3(15.f, 1.f, 15.f);

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;

protected:
	VoxRenderer* m_pVoxRenderer = nullptr;
	VoxAnimator* m_pVoxAnimator = nullptr;
	PhysicsBody* m_pPhysicsBody = nullptr;
	BoxCollider* m_pBoxCollider = nullptr;
	std::unordered_map<std::string, VoxModel*> m_mAnimations;

	/* The player's current direction in world space */
	Vector3 m_Direction = Vector3(0.f, 0.f, 0.f);

	/* The player's current velocity in world space */
	Vector3 m_Velocity = Vector3(0.f, 0.f, 0.f);

	bool m_bIsDead = false;

	float m_fHealth = 0.0f;
	float m_fMaxHealth = 1.0f;

	float m_bAliveTimer = 0.f;

	Vector3 m_DamageOffset = Vector3(0.f);
	float m_fDamageTimer = 0.0f;
	float m_fInvincibilityTimer = 0.8f;
	float m_fDamageKnockBack = 150.f;

	bool m_bCoolDown = false;
	float m_fCoolDownResetTimer = 0.3f;
	float m_fCoolDownTimer = m_fCoolDownResetTimer;

	/*!
	 * @brief used for movement while attacking
	 */
	Vector3 m_AttackOffset = Vector3(0.f);
	bool m_bAttacking = false;

	float m_fAttackTimer = 0.0f;
	const float m_fMaxAttackTimer = 0.2f;

	// TODO: remove when ship game
	/*
	 * @brief this let the entity be immortal from all enemies
	 */
	bool m_bImmortal = false;

	AudioSource* m_pAudioSource = nullptr;
	AudioPlaylist* m_pAudioPlaylist = nullptr;

	FlashBehavior* m_pFlashingComponent = nullptr;

private:
	void AddComponents();

	typedef Entity Base;

	GameManager* m_pGameManager = nullptr;
};

