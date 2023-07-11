#pragma once
#include "Weapon.h"

#include "General/Managers/GameManager.h"
#include "General/Managers/WeaponManager.h"
#include "Core/ECS/Entity.h"

class Player;
class WeaponManager;
class VoxRenderer;
class AudioSource;

class Bullet : public Entity {
public:
	Bullet(World* world);

	void Start() override;

	void SetDamageAmount(float fDamage) { m_fBulletDamage = fDamage; }
	float GetDamageAmount() const { return m_fBulletDamage; };
	
	void FixedTick(const GameTimer& gameTimer) override;
	void OnDrawGizmos(float fDeltaTime) override;

	void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) override;
	void OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled) override;

	void SetCurrentCaster(Player* pPlayer);
	Player* GetCurrentCaster() const;

	void SetCurrentReceiver(Player* pPlayer);
	Player* GetCurrentReceiver() const;

	float GetSpeed() const { return m_fCurrentSpeed; }
	float GetMaxSpeed() const { return m_fMaxSpeed; }
	void SetSpeed(float fSpeed)
	{
		m_fMaxSpeed = fSpeed;
		m_fCurrentSpeed = fSpeed;
	}

	void SetCatchRange(float fRange) { m_fCatchRange = fRange; }
	void SetAutoCatchRange(float fRange) { m_fAutoCatchRange = fRange; }
	void SetPerfectCatchRange(float fRange) { m_fPerfectCatchRange = fRange; }
	void SetMinPerfCatchSpeed(float fRange) { m_fMinPerfCatchSpeed = fRange; }

	bool CanCatchPerfectly() const { return m_fCurrentSpeed >= m_fMinPerfCatchSpeed; };

	float GetCatchRange() const { return m_fCatchRange; };
	float GetAutoCatchRange() const { return m_fAutoCatchRange; };

	float GetPerfectCatchRange() const { return m_fPerfectCatchRange; };
	float GetMinPerfCatchSpeed() const { return m_fMinPerfCatchSpeed; };

	float GetTravelTime() const { return m_fTravelTimer; }

	bool IsAnimationActivated() const { return m_bAnimationActivated; }
	void SetAnimationActivated(bool bActivated) { m_bAnimationActivated = bActivated; }

	float GetPathAnimationTime() const { return m_fPathAnimationTime; }
	uint32_t GetPathAnimationLoopMax() const { return m_uiPathAnimationLoopMax; }

	void SetEscapePosition(Vector3 fPos) { m_EscapePosition = fPos; }
	void SetEscaped(bool fEscaped) { m_bEscaped = fEscaped; }
	bool IsEscaped() const { return m_bEscaped; }
	bool GetEscapeRange() const { return m_fEscapeRange; }

	bool IsMarkedAsDestroy() const {
		return m_bShouldDestroy;
	}

	void MarkAsDestroyed();

	//This is the default model ID, which is compared when the bullet collides with a destructible model
	uint64_t currentModelID = -1;
	bool bIsReturning = false;

	float m_fCameraShake = 0.1f;
	float m_fMaxCameraShake = 0.6f;

	float m_fMinimalBulletSpeed = 0.1f;
	float m_fBulletVoxelHeight = 1.0f;
	
	uint32_t m_uiCheat = 0;

protected:
	typedef Entity Base;

	void ApplyCameraShake() const;

	bool m_bShouldDestroy = false;

	Vector3 m_ScaleAtDestroy = Vector3(1.f, 1.f, 1.f);
	Vector3 m_PositionAtDestroy = Vector3(0.f);

	float m_fDestroyScaleTime = 0.3f;
	float m_fDestroyScaleTimer = m_fDestroyScaleTime;

	float m_fCatchRange = 20.f;
	float m_fPerfectCatchRange = 20.f;
	float m_fAutoCatchRange = 20.0f;

	float m_fMinPerfCatchSpeed = 10.f;

	//The escape variables are used to make sure that if the receiver stood in autocatch range, it won't immidiately pick up the projectile
	Vector3 m_EscapePosition;
	float m_fEscapeRange = 10.f;
	bool m_bEscaped = true;

	float m_fTravelTimer = 0.f;
	float m_fTravelTime = 1.f;

	float m_fDecayTime = 1.4f;

	WeaponManager::Type::MovementType m_MovementType = WeaponManager::Type::MT_STRAIGHT;

	float m_fPathAnimationTimer = 0.f;
	float m_fPathAnimationTime = 1.f;
	float m_fPathAnimationAmplitude = 1.0f;

	bool m_bAnimationActivated = false;

	uint32_t m_uiPathAnimationLoopMax = 0;
	uint32_t m_uiPathAnimationLoopCount = 0;

	Vector3 m_Position = Vector3(0.f);
	Vector3 m_AnimationOffset = Vector3(0.f);

	//Check if the bullet can be destroyed in runtime
	bool m_bCanBeDestroyed = false;

	//How fast the bullet can travel
	float m_fMaxSpeed = 300.0f;

	//How fast the bullet travels
	float m_fCurrentSpeed = 250.0f;

	//How fast the bullet rotates
	float m_fRotationSpeed = 6.0f;

	//This counts how long the bullet has been in existence after spawning
	float m_fLifeTime = 0.0f;

	//Check whether the bullet can be destroyed based on time
	bool m_bTimedDestruction = false;

	//This is how long a bullet can exist in the world, after which it will be destroyed
	float m_fDestroyTimer = 4.0f;

	//The amount of damage a bullet does to an enemy
	float m_fBulletDamage = 25.0f;

	//The range a bullet destroys
	float m_fBulletExplosionRange = 18.0f;

	Vector3 m_Velocity = Vector3(0.0f);
	Vector3 m_Direction = Vector3(0.f, 0.f, 1.f);
	Vector3 m_Right = Vector3(1.f, 0.f, 0.f);

	Player* m_pCurrentPlayerCaster = nullptr;
	Player* m_pCurrentPlayerReceiver = nullptr;

	Weapon* m_pWeapon = nullptr;
	VoxRenderer* m_pVoxRender = nullptr;
	
	GameManager* m_pGameManager = nullptr;
	WeaponManager* m_pWeaponManager = nullptr;

	AudioSource* m_pAudioSource = nullptr;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;
};
