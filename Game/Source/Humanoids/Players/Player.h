#pragma once
#include "../Humanoid.h"
#include "Weapons/Weapon.h"
#include "AI/IFiniteManager.h"
#include "General/Managers/GameManager.h"
#include "AI/FiniteStateMachine.h"

class InputHandler;
class Camera;

class Bullet;

class AimPrefab;
class RecallPrefab;

class Player : public Humanoid, public IFiniteManager<Player>
{
public:

	std::string m_idleAnimation;
	int m_iIdleAnimationFPS = 10;
	std::string m_movingAnimation;
	int m_iMovingAnimationFPS = 15;
	std::string m_dashAnimation;
	int m_iDashAnimationFPS = 63;
	std::string m_throwAnimation;
	int m_iThrowAnimationFPS = 30;
	float m_fThrowAnimationTime = 0.2f;

	float fReturnSpeed = 300.0f;
	bool m_bIsDashing = false;

	bool IsAlive() const;

protected:
	GameManager* m_pGameManager = nullptr;
	Camera* m_pCamera = nullptr;

	InputHandler* m_pInputHandler = nullptr;
	Vector2 m_MovementInput = Vector2(0.f, 0.f);
	Vector2 m_RotationInput = Vector2(0.f, 0.f);
	float m_fJumpForce = 100.f;

	float m_fMovementSpeed = 250.0f;
	float m_fDashSpeed = 700.f;
	float m_fDashDuration = 0.19f;
	float m_fDashCooldown = 0.7f;

	bool m_bIsReceiver = false;

	AimPrefab* m_pAimEntity = nullptr;
	RecallPrefab* m_pRecallEntity = nullptr;
	Weapon* m_pWeapon = nullptr;

	std::vector<Bullet*> m_vCastedBullets;
	std::vector<Bullet*> m_vIncomingBullets;
	
	float m_fPerfectCatchMargin = 1.f;

private:
	float m_fRumbleTimer = 0.25f;
	float m_fRumbleTime = m_fRumbleTimer;
	
	float m_fCatchCooldownTimer = 0.f;
	float m_fDashCooldownTimer = 0.f;

	Bullet* m_pBullet = nullptr;

	bool m_bReturn = false;
	bool m_bRumble = false;
	bool m_bIsRumbling = false;
public:
	Player(World* pWorld);
	virtual ~Player();

	virtual void Awake() override;
	virtual void Start() override;
	virtual void Tick(float fDeltaTime) override;
	virtual void FixedTick(const GameTimer& gameTimer) override;

	void SetMovementSpeed(float fMoveSpeed) { m_fMovementSpeed = fMoveSpeed; }
	float GetMovementSpeed() const { return m_fMovementSpeed; }

	bool Damage(float damage, Vector3 impactNormal, float fLaunchStrength = 1.0f) override;
	Weapon* GetCurrentWeapon() const { return m_pWeapon; }
	void AddSpawnedBullet(Bullet* pBullet);
	void RemoveSpawnedBullet(Bullet* pBullet);

	float GetCurrentDashCooldownTimer() const { return m_fDashCooldownTimer; }
	float GetDashCooldownDuration() const { return m_fDashCooldown; }
	float GetDashSpeed() const { return m_fDashSpeed; }
	float GetDashDuration() const { return m_fDashDuration; }
	bool IsDashing() const { return GetFSM()->GetCurrentState() == GetState("Dashing"); }
	void Switch();

	Vector2 GetRotationInput() { return m_RotationInput; };

	void SetLinkPlayer(Player* pPlayer)
	{
		m_pReferencePlayer = pPlayer;
	}
	Player* GetLinkedPlayer() const { return m_pReferencePlayer; }

	void ShowAimer();
	void HideAimer();

	bool IsMerged() const { return (m_pMergeWith != nullptr); }
	virtual void Dead() override;

protected:
	void Catch();

private:
	void AddComponents();

	// Merge with P1
	Entity* m_pMergeWith = nullptr;

	float m_fBGMVolume = 1.f;

	bool m_bIsDead = false;

	bool m_bIsAltPlayer = false;
	bool m_bGamePaused = false;

	Player* m_pReferencePlayer = nullptr;

	RTTR_ENABLE(Humanoid)
	RTTR_REGISTRATION_FRIEND;
};

