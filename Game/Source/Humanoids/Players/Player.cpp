#include "Player.h"

#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Entities/Camera.h>

#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/VoxAnimator.h>
#include <Core/ECS/Components/SpriteRenderer.h>

#include <Core/LoggingSystem/LoggingSystem.h>

#include "Prefabs/AimPrefab.h"
#include "Prefabs/RecallPrefab.h"
#include "Weapons/Bullet.h"

#include <External/rttr/registration.h>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include <Core/GameTimer.h>
#include <Core/Resources/Formats/VoxModel.h>

#include <Core/ECS/Components/InputHandler.h>

#include "UI/WorldSwitch.h"
#include "UI/MainMenu/MainMenuManagerComponent.h"

#include "Humanoids/States/Hum_IdleState.h"
#include "Humanoids/States/Hum_MoveState.h"
#include "Humanoids/States/Hum_DashState.h"
#include "Core/ECS/Components/AudioSource.h"
#include "Humanoids/States/Hum_ThrowState.h"
#include "Humanoids/ParticleCorpse.h"
#include <Core/Platform/Audio/AudioContext.h>
#include "Humanoids/Humanoid.h"
#include "Core/ECS/Components/PhysicsBody.h"
#include "General/FlashBehavior.h"

#define PAUSE_MENU_WORLD_FILE "Content/Worlds/Menus/Pause_Menu.wld"
#define MAIN_MENU_WORLD_FILE "Content/Worlds/Menus/Main_Menu.wld"
#define GAME_OVER_SCREEN_WORLD_FILE "Content/Worlds/Menus/Game_Over_Screen.wld"

RTTR_REGISTRATION
{
	rttr::registration::class_<Player>("Player")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Movement Speed", &Player::GetMovementSpeed, &Player::SetMovementSpeed)(RTTR_PUBLIC)
	.property("Jump Force", &Player::m_fJumpForce)(RTTR_PUBLIC)
	.property("Dash Speed", &Player::m_fDashSpeed)(RTTR_PUBLIC)
	.property("Dash Duration", &Player::m_fDashDuration)(RTTR_PUBLIC)
	.property("Dash Cooldown", &Player::m_fDashCooldown)(RTTR_PUBLIC)
	.property("Perfect Catch Margin", &Player::m_fPerfectCatchMargin)(RTTR_PUBLIC)
	.property("Bullet Return Speed", &Player::fReturnSpeed)(RTTR_PRIVATE)
	.property("Knockback Force", &Humanoid::m_fDamageKnockBack)(RTTR_PUBLIC)
	.property("Idle Animation", &Player::m_idleAnimation)(RTTR_PUBLIC)
	.property("Idle Animation FPS", &Player::m_iIdleAnimationFPS)(RTTR_PUBLIC)
	.property("Moving Animation", &Player::m_movingAnimation)(RTTR_PUBLIC)
	.property("Moving Animation FPS", &Player::m_iMovingAnimationFPS)(RTTR_PUBLIC)
	.property("Dash Animation", &Player::m_dashAnimation)(RTTR_PUBLIC)
	.property("Dash Animation FPS", &Player::m_iDashAnimationFPS)(RTTR_PUBLIC)
	.property("Throw Animaton", &Player::m_throwAnimation)(RTTR_PUBLIC)
	.property("Throw Animaton FPS", &Player::m_iThrowAnimationFPS)(RTTR_PUBLIC)
	.property("Throw Animaton Time", &Player::m_fThrowAnimationTime)(RTTR_PUBLIC)
	.property("Invincibility Time", &Humanoid::m_fInvincibilityTimer)(RTTR_PUBLIC);
}

bool Player::IsAlive() const
{
	return (m_pGameManager) ? m_pGameManager->GetSharedPlayerHealth() : Humanoid::IsAlive();
}

Player::Player(World* pWorld) : Humanoid(pWorld)
{
	SetName("Player");

	m_pFiniteStateMachine = new FiniteStateMachine<Player>(this);
	AddState({ "Idle", new Hum_IdleState() });
	AddState({ "Moving", new Hum_MoveState() });
	AddState({ "Dashing", new Hum_DashState() });
	AddState({ "Throw" , new Hum_ThrowState() });
	

}

Player::~Player()
{
	if (m_pInputHandler)
		m_pInputHandler->VibrateGamePad(0.0f, 0.0f);
}

void Player::Awake()
{
	Humanoid::Awake();

	/* Add tag */
	auto& tags = GetTags();
	if (std::find(tags.begin(), tags.end(), "Player") == tags.end())
		tags.push_back("Player");

	/* Aim/Recall child entity */
	for (Entity* pChild : GetChildren())
	{
		if (pChild->GetName() == "Aim")
		{
			m_pAimEntity = dynamic_cast<AimPrefab*>(pChild);
		}

		if (pChild->GetName() == "Recall")
		{
			m_pRecallEntity = dynamic_cast<RecallPrefab*>(pChild);
		}

		if (m_pAimEntity && m_pRecallEntity)
			break;

	}

	if (!m_pAimEntity)
	{
		m_pAimEntity = GetWorld()->SpawnEntity<AimPrefab>(Vector3(), Quaternion(), Vector3());
		m_pAimEntity->SetParent(this);
	}

	if (!m_pRecallEntity)
	{
		m_pRecallEntity = GetWorld()->SpawnEntity<RecallPrefab>(Vector3(), Quaternion(), Vector3());
		m_pRecallEntity->SetParent(this);
	}

	AddComponents();
	SetMovementSpeed(200.f);
	m_iThrowAnimationFPS = 33.f;
	m_fThrowAnimationTime = 0.12f;
}

void Player::Start()
{
	Humanoid::Start();
	AddComponents();
	SetMovementSpeed(200.f);
	m_iThrowAnimationFPS = 33.f;
	m_fThrowAnimationTime = 0.12f;
	GetComponentAll<BoxCollider>()->SetTrigger(false);

	GetWorld()->Paused += Event<World*>::Subscriber([this](World* pWorld)
	{
		if (m_pInputHandler)
			m_pInputHandler->VibrateGamePad(0.0f, 0.0f);
	}, this);

	bool bIsPlayer1 = GetName() == "Player";
	m_bIsAltPlayer = GetName() == "Player1";

	if (bIsPlayer1)
	{
		m_idleAnimation = "Content/Character_Models/Player/Main_Char_Female_Idle.anim.vox";
		m_movingAnimation = "Content/Character_Models/Player/Main_Char_Female_Walk_FastPaced.anim.vox";
		m_dashAnimation = "Content/Character_Models/Player/Main_Char_Female_Dash.anim.vox";
		m_throwAnimation = "Content/Character_Models/Player/Main_Char_Female_Throw_w_Effect.anim.vox";
	}

	else if (m_bIsAltPlayer)
	{
		m_idleAnimation = "Content/Character_Models/Player/Main_Char_Male_Idle.anim.vox";
		m_movingAnimation = "Content/Character_Models/Player/Main_Char_Male_Walk_FastPaced.anim.vox";
		m_dashAnimation = "Content/Character_Models/Player/Main_Char_Male_Dash.anim.vox";
		m_throwAnimation = "Content/Character_Models/Player/Main_Char_Male_Throw.anim.vox";
	}

	// Load animations
	if (bIsPlayer1 || m_bIsAltPlayer)
	{
		std::vector<std::string> animations;
		if (!m_idleAnimation.empty())
			animations.push_back(m_idleAnimation);
		if (!m_movingAnimation.empty())
			animations.push_back(m_movingAnimation);
		if (!m_dashAnimation.empty())
			animations.push_back(m_dashAnimation);
		if (!m_throwAnimation.empty())
			animations.push_back(m_throwAnimation);

		m_pVoxAnimator->SetAnimationFiles(animations);
	}
	else
	{
		std::vector<std::string> animations = m_pVoxAnimator->GetAnimationFiles();
		m_pVoxAnimator->SetAnimationFiles(animations);
	}

	m_pBoxCollider->SetContinuousCollision(true);
	m_pBoxCollider->AutoFit();

	m_bReturn = false;

	m_pGameManager = dynamic_cast<GameManager*>(GetWorld()->FindEntity("GameManager"));

	// Enable aim marker if ammo is not zero
	const bool bIsCaster = m_pWeapon->GetCurrentAmmo() != 0 || m_pWeapon->HasInfiniteAmmo();
	m_bIsReceiver = !bIsCaster;
	if (m_pReferencePlayer) m_pReferencePlayer->m_bIsReceiver = bIsCaster;
	m_pAimEntity->SetEnabled(bIsCaster);
	m_pRecallEntity->SetEnabled(!bIsCaster);


	/* Configure input */
	m_pInputHandler->BindAction("Dash", IKS_PRESSED, [&]() 
	{
		if (m_fDashCooldownTimer <= 0.f && GetFSM()->GetCurrentState() == GetState("Moving"))
		{
			SetState("Dashing");

			m_fDashCooldownTimer = m_fDashCooldown;
			m_fCatchCooldownTimer = m_fPerfectCatchMargin;
		}
	});

	m_pInputHandler->BindAction("Special", IKS_PRESSED, [&]()
	{
		for (Bullet* pBullet : m_vIncomingBullets)
		{
			if (pBullet->IsMarkedAsDestroy())
				continue;

			m_pBullet = pBullet;
			m_pBullet->bIsReturning = true;
			m_bReturn = true;
			m_pBullet->SetEscaped(true);

			m_pBullet->m_uiCheat = m_pBullet->m_uiCheat == 0 ? 1 : 0;

			// Play catch audio
			m_pAudioSource->SetLooping(false);

			m_pAudioSource->SetFilePath("Content/SFX/PlayerRecallSound.ogg");
			m_pAudioSource->Play();
		}
	});

	m_pInputHandler->BindAction("Fire", IKS_PRESSED, [&]() 
	{
		// If you are the receiver
		if(m_bIsReceiver && m_pWeapon->GetCurrentAmmo() == 0)
		{
			if (m_bReturn) // return is the bullet is return to the other player.
				return;
			{
				GetComponent<VoxAnimator>()->SetFPS(10);
			}

			for (Bullet* pBullet : m_vIncomingBullets)
			{
				if (pBullet->IsMarkedAsDestroy())
					continue;

				m_pBullet = pBullet;
				m_pBullet->bIsReturning = true;
				m_bReturn = true;
				m_pBullet->SetEscaped(true);

				// Play catch audio
				m_pAudioSource->SetLooping(false);

				m_pAudioSource->SetFilePath("Content/SFX/PlayerRecallSound.ogg");
				m_pAudioSource->Play();
			}

			if (m_pWeapon->GetCurrentAmmo() > 0 || m_pWeapon->HasInfiniteAmmo())
			{
				ShowAimer();
			}

			m_vIncomingBullets.clear();
			if (m_pReferencePlayer) m_pReferencePlayer->m_vIncomingBullets.clear();
		} 
		else // If you are not the receiver do this part
		{
			if (m_pWeapon) // only fire if we have zero bullets in the world.
			{
				SetState("Throw");

				//timer / delay
				
				   
				/* Make all existing bullets (that the current player cast) animate when pressing fire */
				for (Bullet* pBullet : m_vCastedBullets)
				{
					if (pBullet->IsMarkedAsDestroy())
						continue;

					pBullet->SetAnimationActivated(true);
				}

				/* Fire */
				//m_pWeapon->Fire();

				/* Switch the player from casting to receiving */
				Switch();

				// Reset rumbling 
				m_pInputHandler->VibrateGamePad(0.0f, 0.0f);
				m_bRumble = false;
				m_fRumbleTime = m_fRumbleTimer;
				m_bIsRumbling = false;
			}
		}
	});

	m_pInputHandler->BindAction("Pause_Game", IKS_PRESSED, [&]()
	{
		m_bGamePaused = true;

		GetWorld()->GetRenderSystem()->SetFadeTime(0.2f);
		GetWorld()->GetRenderSystem()->Fade();
	});

	if (m_pAimEntity->pAimRenderer && m_pInputHandler->GetPlayerHandle() == 2) m_pAimEntity->pAimRenderer->SetOverrideColor
	(
		VColor(static_cast<unsigned char>(24u), 59u, 242u, 255u)
	);

	if (m_pRecallEntity->pArrowRenderer && m_pInputHandler->GetPlayerHandle() == 2) m_pRecallEntity->pArrowRenderer->SetColor
	(
		VColor(static_cast<unsigned char>(24u), 59u, 242u, 255u)
	);

	m_pRecallEntity->SetEnabled(false);

	m_bGamePaused = false;

	m_pCamera = GetWorld()->GetMainCamera();
	m_pCamera->Destroyed += Event<Entity*>::Subscriber([this](Entity* pEntity)
	{
		m_pCamera = nullptr;
	}, this);

	SetState("Idle");

	// Single player mode
	if (MainMenuManagerComponent::m_uiPlayerCount == 1)
	{
		if (
			(m_bIsAltPlayer && MainMenuManagerComponent::m_bPlayer1Active) ||
			(!m_bIsAltPlayer && MainMenuManagerComponent::m_bPlayer2Active)
		)
		{
			m_pMergeWith = GetWorld()->FindEntity(m_bIsAltPlayer ? "Player" : "Player1");

			if (m_pMergeWith)
			{
				m_pBoxCollider->SetTrigger(true);
				m_pBoxCollider->SetBoxSize(Vector3(FLT_MIN, FLT_MIN, FLT_MIN));
				m_pPhysicsBody->SetEnabled(false);
				m_pVoxRenderer->SetEnabled(false);
				m_pAudioSource->SetEnabled(false);

				m_pInputHandler->SetPlayerHandle(m_bIsAltPlayer ? 1 : 2);

				GetTransform()->SetPosition(m_pMergeWith->GetTransform()->GetPosition());
			}
		}
	}
}

void Player::Tick(float fDeltaTime)
{
	if (m_pMergeWith)
	{
		GetTransform()->SetPosition(m_pMergeWith->GetTransform()->GetPosition());
	}

	if (m_bIsDead)
	{
		float fFadeValue =  GetWorld()->GetRenderSystem()->GetFadeValue();

		AudioContext* pAudioContext = GetWorld()->GetApplication()->GetPlatform().GetAudioContext();
		pAudioContext->SetBGMVolume(
			m_fBGMVolume * fFadeValue
		);

		if (GetWorld()->GetRenderSystem()->IsFaded())
		{
			pAudioContext->StopBGM();

			pAudioContext->SetBGMVolume(
				0.75f
			);

			WorldSwitch::SwitchWorld(GetWorld(), GAME_OVER_SCREEN_WORLD_FILE, false, false);
		}

		return;
	}

	if (GetName() == "Player")
	{
		if (m_bGamePaused && GetWorld()->GetRenderSystem()->IsFaded())
		{
			m_bGamePaused = false;
			GetWorld()->OpenWorld(PAUSE_MENU_WORLD_FILE, false);
		}
	}

	Humanoid::Tick(fDeltaTime);
	m_pFiniteStateMachine->Tick(fDeltaTime);

	if (m_fDashCooldownTimer > 0.f)
		m_fDashCooldownTimer -= fDeltaTime;

	/* Update input */
	m_MovementInput.x = m_pInputHandler->GetAxisValue("MoveRight");
	m_MovementInput.y = m_pInputHandler->GetAxisValue("MoveForward");

	m_RotationInput.x = m_pInputHandler->GetAxisValue("RotateRight");
	m_RotationInput.y = m_pInputHandler->GetAxisValue("RotateUp");

	/* Catch */
	if (!m_pWeapon->HasInfiniteAmmo() && m_pWeapon->GetCurrentAmmo() == 0 && m_bIsReceiver)
	{
		Catch();
	}

	/* Recall */
	if (m_bReturn && m_pBullet && m_bIsReceiver)
	{
		// Set our position as a fraction of the distance between the markers.
		m_pBullet->GetTransform()->SetPosition(Utils::MoveTowards(m_pBullet->GetTransform()->GetPosition(), GetTransform()->GetPosition(), fDeltaTime * fReturnSpeed));
	}

	/* if there is no bullet at all we need to feed it someone */
// 	if(bIsReceiver && (m_vIncomingBullets.empty() && m_vCastedBullets.empty()))
// 	{
// 		m_pWeapon->SetCurrentAmmo(m_pWeapon->GetCurrentAmmo() + 1);
// 		m_bRumble = true;
// 	}

	// Note for debug purposes
	// TODO adding recall feature automatic after n seconds
	if(m_vIncomingBullets.size() > 1 || m_vCastedBullets.size() > 1)
	{
		// GetWorld()->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_MESSAGE, "There more than one bullet in the scene");
	}
}

void Player::FixedTick(const GameTimer& gameTimer)
{
	m_pFiniteStateMachine->FixedTick(gameTimer);
	float fDeltaTime = static_cast<float>(gameTimer.GetElapsedSeconds());

	bool bIsMoved = m_MovementInput != Vector2(0.f, 0.f);
	bool bIsRotated = m_RotationInput != Vector2(0.f, 0.f);

	/* Catch cooldown timer when not dashing */
	if (m_fCatchCooldownTimer > 0.f)
	{
		bool bIsDashing = m_pFiniteStateMachine->GetCurrentState() == GetState("Dashing");
		if (!bIsDashing) m_fCatchCooldownTimer -= fDeltaTime;
	}

	if (bIsMoved && m_pCamera)
	{
		m_Direction = glm::rotate(
			m_pCamera->GetTransform()->GetRotation(), Vector3(m_MovementInput.x, 0.f, m_MovementInput.y)
		);

		/* Don't float in air */
		m_Direction.y = 0.f;
		m_Direction = glm::normalize(m_Direction);

		//std::cout << m_Velocity.x << " " << m_Velocity.y << " " << m_Velocity.z << std::endl;

		m_Velocity = m_Direction * m_fMovementSpeed;
	}
	else
	{
		m_Velocity = Vector3(0.f, 0.f, 0.f);
	}

	if (bIsRotated && m_pCamera)
	{
		m_Direction = glm::rotate(
			m_pCamera->GetTransform()->GetRotation(), Vector3(m_RotationInput.x, 0.f, m_RotationInput.y)
		);

		/* Don't float in air */
		m_Direction.y = 0.f;
		m_Direction = glm::normalize(m_Direction);
	}

	if(m_bRumble)
	{
		if (!m_bIsRumbling)
		{
			m_pInputHandler->VibrateGamePad(1.0f, 1.0f);
			m_bIsRumbling = true;
		}

		m_fRumbleTime -= fDeltaTime;
		if (m_fRumbleTime < 0.0f) 
		{
			m_pInputHandler->VibrateGamePad(0.0f, 0.0f);
			m_fRumbleTime = m_fRumbleTimer;
			m_bRumble = false;
			m_bIsRumbling = false;
		}
	}


	Humanoid::FixedTick(gameTimer);
}

void Player::AddSpawnedBullet(Bullet* pBullet)
{
	m_vCastedBullets.push_back(pBullet);

	if(m_pReferencePlayer)
		m_pReferencePlayer->m_vIncomingBullets.push_back(pBullet);
}


void Player::RemoveSpawnedBullet(Bullet* pBullet)
{
	auto it = std::find(m_vCastedBullets.begin(), m_vCastedBullets.end(), pBullet);

	if (it != m_vCastedBullets.end())
	{
		m_vCastedBullets.erase(it);

		if (!m_pWeapon->HasInfiniteAmmo() && m_pWeapon->GetCurrentAmmo() == 0)
		{
			HideAimer();
		}
	}

	it = std::find(m_vIncomingBullets.begin(), m_vIncomingBullets.end(), pBullet);

	if (it != m_vIncomingBullets.end())
	{
		m_vIncomingBullets.erase(it);
	}

	if(m_pBullet == pBullet)
	{
		m_pBullet = nullptr;
	}
}

void Player::Switch()
{
	// So now you are the thrower and the other one is the receiver now
	m_bIsReceiver = false;
	m_pRecallEntity->SetEnabled(false);
	if (m_pReferencePlayer)
	{
		m_pReferencePlayer->m_pRecallEntity->SetEnabled(true);
		m_pReferencePlayer->m_bIsReceiver = true;
	}
}

bool Player::Damage(float damage, Vector3 impactNormal, float fLaunchStrength /*= 1.0f*/)
{
	if (!m_pGameManager->CanBeDamaged() || m_bIsDashing || m_pMergeWith)
		return false;

	m_pGameManager->SharedPlayerHealthTakeDamage(static_cast<int>(damage));

	// Don't, we use the shared health system
	// Humanoid::Damage(damage, impactNormal, fLaunchStrength);

	if (m_pFlashingComponent)
		m_pFlashingComponent->StartFlashing();

	m_DamageOffset = impactNormal;

	// TODO: see why this should be 1 
	m_DamageOffset = glm::normalize(impactNormal);
	m_DamageOffset.y = 1.f;
	m_DamageOffset *= fLaunchStrength;

	m_pPhysicsBody->ApplyImpulse(m_DamageOffset);

	if (!IsAlive())
	{
		Dead();
	}

	// Play damage audio
	m_pAudioSource->SetLooping(false);

	m_pAudioSource->SetFilePath(m_bIsAltPlayer ? "Content/SFX/PlayerMaleHit.ogg" : "Content/SFX/PlayerFemaleHit.ogg");
	m_pAudioSource->Play();

	return true;
}

void Player::ShowAimer()
{
	m_pAimEntity->SetEnabled(true);
}

void Player::HideAimer()
{
	m_pAimEntity->SetEnabled(false);
}

void Player::Dead()
{
	if (m_bIsDead)
		return;

	ParticleCorpse* particleEntity = GetWorld()->SpawnEntity<ParticleCorpse>(GetTransform()->GetPosition(), GetTransform()->GetRotation(), GetTransform()->GetScale());
	particleEntity->m_voxFile = m_pVoxRenderer->GetModelFilePath();
	particleEntity->m_MinForce = v3MaxForce;
	particleEntity->m_MaxForce = -v3MaxForce;
	particleEntity->m_fArcAngle = 180.f;

	m_pVoxRenderer->SetEnabled(false);

	m_fBGMVolume = GetWorld()->GetApplication()->GetPlatform().GetAudioContext()->GetBGMVolume();

	m_bIsDead = true;
	GetWorld()->GetRenderSystem()->Fade();
}

void Player::Catch()
{
	std::vector<Entity*> pBullets = GetWorld()->FindEntitiesWithTag("Bullet");
	bool bCaught = false;

	// Check if the catch can be perfect
	const bool bCanBePerfect = m_pFiniteStateMachine->GetCurrentState() == GetState("Dashing") || m_fCatchCooldownTimer > 0.f;

	for (Entity* pEntity : pBullets)
	{
		auto pBullet = dynamic_cast<Bullet*>(pEntity);

		if (pBullet->GetCurrentCaster() == this || pBullet->GetTravelTime() < 0.33f || pBullet->IsMarkedAsDestroy())
			continue;

		const float fAutoRange = pBullet->GetAutoCatchRange();
		const float fPerfectRange = pBullet->GetPerfectCatchRange();
		const float fRange = pBullet->GetCatchRange();

		float fDistance = glm::distance(GetTransform()->GetPosition(), pBullet->GetTransform()->GetPosition());

		bool bEscaped = pBullet->IsEscaped();

		if ((fDistance < fAutoRange || fDistance < fRange) && bEscaped && pBullet->m_uiCheat == 0)
		{
			if(m_bReturn)
				m_pGameManager->ResetComboTimer();

			//If the combo gathered is greater than the initial combo when thrown
			if (m_pGameManager->comboOnCatch > m_pGameManager->comboOnThrow)
			{
				//The new initial combo when thrown will be set to the combo gathered in the current projectile
				m_pGameManager->comboOnThrow = m_pGameManager->comboOnCatch;

				//The combo gathered has to be reset to 0, for the next throw/fire
				m_pGameManager->comboOnCatch = 0;
			}
			//If not enough combo is gathered in the current throw/fire
			else
			{
				//Reset the combo streak
				m_pGameManager->ResetComboStreak();

				//Both combo variables have to be reset as well
				m_pGameManager->comboOnThrow = 0;
				m_pGameManager->comboOnCatch = 0;
			}

			m_bRumble = true;

			if (m_pBullet == pBullet && m_bReturn)
			{
				m_pBullet = nullptr;
				m_pReferencePlayer->m_pBullet = nullptr;
				m_bReturn = false;
			}

			pBullet->MarkAsDestroyed();

			if (bCanBePerfect && fDistance < fPerfectRange && pBullet->CanCatchPerfectly())
			{
				if (m_pWeapon->GetCurrentAmmo() == 0)
					m_pWeapon->SetCurrentAmmo(m_pWeapon->GetCurrentAmmo() + 1);
				bCaught = true;

				m_fDashCooldownTimer = 0.f;

				GetWorld()->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_MESSAGE, "Perfect Catch");
				continue;
			}

			if (m_pWeapon->GetCurrentAmmo() == 0)
				m_pWeapon->SetCurrentAmmo(m_pWeapon->GetCurrentAmmo() + 1);
			
			bCaught = true;

			// Play catch audio
			m_pAudioSource->SetLooping(false);

			m_pAudioSource->SetFilePath("Content/SFX/PlayerCatch.ogg");
			m_pAudioSource->Play();

			GetWorld()->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_MESSAGE, "Catch");
		}
	}

	m_pAimEntity->SetEnabled(bCaught);
}

void Player::AddComponents()
{
	// InputHandler
	m_pInputHandler = GetComponent<InputHandler>();
	if (!m_pInputHandler)
		m_pInputHandler = AddComponent<InputHandler>();

	// Weapon
	m_pWeapon = GetComponent<Weapon>();
	if (!m_pWeapon)
		m_pWeapon = AddComponent<Weapon>();

	// VoxRenderer
	if (m_pVoxRenderer == nullptr)
		m_pVoxRenderer = AddComponent<VoxRenderer>();
	m_pBoxCollider->SetBoxSize(m_pVoxRenderer->GetFrame());

	// VoxAnimator
	if (m_pVoxAnimator == nullptr)
		m_pVoxAnimator = AddComponent<VoxAnimator>();
}
