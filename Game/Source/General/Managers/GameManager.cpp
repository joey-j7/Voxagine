
#include "pch.h"

#include <External/rttr/registration.h>

#include "GameManager.h"

#include <Core/ECS/World.h>
#include "Core/ECS/Components/InputHandler.h"
#include "Core/ECS/Components/Transform.h"
#include "Core/ECS/Components/AudioSource.h"
#include "Core/ECS/Components/UI/UIButton.h"
#include "Core/Platform/Input/InputContext.h"

#include "AI/FiniteStateMachine.h"

#include "UI/Loadout.h"

#include "Humanoids/Enemies/Monster.h"
#include "Humanoids/Players/Player.h"
#include "UI/HealthUI.h"
#include "UI/ComboUI.h"
#include "UI/ComboSliderUI.h"
#include "UI/ComboIcon.h"

#include "Core/MetaData/PropertyTypeMetaData.h"

// States
#include "Gameplay/States/GM_LoadoutState.h"
#include "UI/States/MenuState.h"

RTTR_REGISTRATION
{
	/*
	 * @brief Example enumeration registration with RTTR
	 * @param Class, Registration name
	 * @param <name, EnumValue>
	 */
	rttr::registration::enumeration<EGameState>("GameState")
	(
		rttr::value("Start",	EGameState::Start),
		rttr::value("Play",		EGameState::Play),
		rttr::value("End",		EGameState::End)
	);

	rttr::registration::class_<GameManager>("GameManager")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		)
		.property("Is Playing", &GameManager::m_bIsPlaying)
		.property("State", &GameManager::GetPlayState, &GameManager::SetPlayState)(RTTR_PUBLIC)
		.property("Players", &GameManager::m_pPlayers)(RTTR_PUBLIC)
		.property("Min Voxel Explosion Range", &GameManager::voxelExplosionRangeMin)(RTTR_PUBLIC)
		.property("Max Voxel Explosion Range", &GameManager::voxelExplosionRangeMax)(RTTR_PUBLIC)
		.property("Bullet Return Speed", &GameManager::m_fBulletReturnSpeed)(RTTR_PUBLIC)
		.property("Health", &GameManager::GetHealth, &GameManager::SetHealth)(RTTR_PUBLIC)
		.property("Max Health", &GameManager::GetMaxHealth, &GameManager::SetMaxHealth)(RTTR_PUBLIC)
		.property("Invincibility Hit Delay", &GameManager::m_fInvincibilityTime)(RTTR_PUBLIC)
		.property("Player End Positions", &GameManager::m_vArrEndEntities)(RTTR_PUBLIC)
		.property("ComboTimer Starting Time", &GameManager::comboTimerStartTime)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Time Based Bonus 1", &GameManager::timeBasedBonus1)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Bonus 1 Time held", &GameManager::Bonus1TimeHeld)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Time Based Bonus 2", &GameManager::timeBasedBonus2)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Bonus 2 Time held", &GameManager::Bonus2TimeHeld)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Hitting Geometry Bonus", &GameManager::environmentComboBonus)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Hitting Enemy Bonus", &GameManager::enemyComboBonus)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Combo Threshold 1", &GameManager::comboThreshold1)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Combo Threshold 2", &GameManager::comboThreshold2)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Combo Threshold 3", &GameManager::comboThreshold3)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Speed Combo Multiplier 1", &GameManager::speedComboMultiplier1)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Speed Combo Multiplier 2", &GameManager::speedComboMultiplier2)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("Speed Combo Multiplier 3", &GameManager::speedComboMultiplier3)(RTTR_PUBLIC, RTTR_CATEGORY("ComboSystem"))
		.property("UI buttons", &GameManager::vCurrentButtons)(RTTR_PUBLIC);
}

void GameManager::SetPlayerPosition(const Vector3& vPosition, uint32_t uiIndex)
{
	if (uiIndex < m_pPlayers.size() && m_pPlayers[uiIndex]) m_pPlayers[uiIndex]->GetTransform()->SetPosition(vPosition);
}

void GameManager::SetEndPlayerPosition(const Vector3& vPosition, uint32_t uiIndex)
{
	if (uiIndex < m_vArrEndEntities.size() && m_vArrEndEntities[uiIndex]) m_vArrEndEntities[uiIndex]->GetTransform()->SetPosition(vPosition);
}

GameManager::GameManager(World* world) : Entity(world)
{
	SetName("GameManager");

	m_pFiniteStateMachine = new FiniteStateMachine<GameManager>(this);
	AddState({ "LoadOutCustomization", new GM_LoadOutState }, true);
	AddState({ "Playing", new MenuState }); // Dummy state
}

void GameManager::StartGame()
{
	if (m_pPlayers[0])
	{
		m_pPlayers[0]->SetPersistent(true);
		// m_pPlayers[0]->GetTransform()->SetPosition((m_pPlayers[0] ? m_pPlayers[0]->GetTransform()->GetPosition() : Vector3(0.0f)));
	}
	if (m_pPlayers[1])
	{
		m_pPlayers[0]->SetPersistent(true);
		// m_pPlayers[1]->GetTransform()->SetPosition((m_pPlayers[1] ? m_pPlayers[1]->GetTransform()->GetPosition() : Vector3(0.0f)));
	}

	m_fHealth = m_fMaxHealth;
	m_bIsPlaying = true;
}

void GameManager::Reset()
{
	if (m_pPlayers[0])
		m_pPlayers[0]->Reset();

	if(m_pPlayers[1])
		m_pPlayers[1]->Reset();

	m_bIsPlaying = true;
}

void GameManager::SetPlayState(EGameState state)
{
	if (m_EGameState == state)
		return;

	m_EGameState = state;
	switch (m_EGameState)
	{
	case EGameState::Start:
		SetState("LoadOutCustomization");
		break;
	case EGameState::Play:
		SetState("Playing");
		break;
	case EGameState::End:
		break;
	}
}

void GameManager::Awake()
{
	if (!m_pPlayers[0] && !m_pPlayers[1])
	{
		auto players = GetWorld()->FindEntitiesOfType<Player>();
		if (players.size() == 1)
			m_pPlayers[0] = players[0];
		if (players.size() == 2)
		{
			m_pPlayers[0] = players[0];
			m_pPlayers[1] = players[1];

			m_pPlayers[0]->SetLinkPlayer(m_pPlayers[1]);
			m_pPlayers[1]->SetLinkPlayer(m_pPlayers[0]);

			m_pPlayers[0]->fReturnSpeed = m_fBulletReturnSpeed;
			m_pPlayers[1]->fReturnSpeed = m_fBulletReturnSpeed;
		}
	}

	const auto children = GetChildren();
	if (!children.empty())
	{
		for (auto pChild : children)
		{
			if (pChild->HasTag("End"))
			{
				if (!m_vArrEndEntities[0])
					m_vArrEndEntities[0] = pChild;
				else if (!m_vArrEndEntities[1])
					m_vArrEndEntities[1] = pChild;
			}
		}

		for (auto& pEntity : m_vArrEndEntities)
		{
			if (!pEntity)
			{
				pEntity = GetWorld()->SpawnEntity<Entity>(GetTransform()->GetPosition(), GetTransform()->GetRotation(), Vector3(1.0f));
				pEntity->AddTag("End");
				pEntity->SetParent(this);
			}
		}
	}
	else
	{
		uint32_t counter = 0;
		for (auto& pEntity : m_vArrEndEntities)
		{
			if (!pEntity)
			{
				pEntity = GetWorld()->SpawnEntity<Entity>(GetTransform()->GetPosition(), GetTransform()->GetRotation(), Vector3(1.0f));
				pEntity->AddTag("End");
				pEntity->SetName("EndEntity " + std::to_string(counter++));
				pEntity->SetParent(this);
			}
		}
	}

	if (m_pFiniteStateMachine->GetCurrentState()) m_pFiniteStateMachine->GetCurrentState()->Awake(this);
}

void GameManager::Start()
{
	Entity::Start();

	SetPersistent(true);

	m_pHealthUI = dynamic_cast<HealthUI*>(GetWorld()->FindEntity("HealthUI"));
	m_pComboUI = dynamic_cast<ComboUI*>(GetWorld()->FindEntity("ComboUI"));
	m_pComboSlider = dynamic_cast<ComboSliderUI*>(GetWorld()->FindEntity("ComboSliderUI"));
	m_pComboIcon = dynamic_cast<ComboIcon*>(GetWorld()->FindEntity("ComboIcon"));

	AddComponent<AudioSource>();
	/* TODO grab it for the load out state = */ AddComponent<LoadOut>();

	switch (m_EGameState)
	{
	case EGameState::Start:
		SetState("LoadOutCustomization");
		break;
	case EGameState::Play:
		SetState("Playing");
		break;
	case EGameState::End:
		break;
	}

	m_pInputHandler = AddComponent<InputHandler>();
	if (!m_pInputHandler)
		m_pInputHandler = GetComponent<InputHandler>();

	StartGame();
}

void GameManager::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);

	if (m_fInvincibilityTimer > 0)
		m_fInvincibilityTimer -= fDeltaTime;

	if(m_pFiniteStateMachine)
	{
		m_pFiniteStateMachine->Tick(fDeltaTime);
	}

	if (currentComboTimer > 0)
	{
		currentComboTimer = currentComboTimer - fDeltaTime;
	}
}

void GameManager::OnDrawGizmos(float)
{
	DebugRenderer* pDebugRenderer = GetWorld()->GetDebugRenderer();
	if (pDebugRenderer)
	{
		pDebugRenderer->AddCenteredSphere((m_vArrEndEntities[0] ? m_vArrEndEntities[0]->GetTransform()->GetPosition() : Vector3(0.0f)), Vector3(15.0f), VColors::YellowGreen);
		pDebugRenderer->AddCenteredSphere((m_vArrEndEntities[1] ? m_vArrEndEntities[1]->GetTransform()->GetPosition() : Vector3(0.0f)), Vector3(15.0f), VColors::YellowGreen);
	}
}

void GameManager::ResetComboTimer()
{
	currentComboTimer = comboTimerStartTime;
}

int GameManager::GetComboStreak()
{
	if (m_pComboUI)
	{
		m_pComboUI->SetComboUI(std::to_string(comboStreak));
	}
	return comboStreak;
}

float GameManager::GetComboTimer()
{
	return currentComboTimer;
}

int GameManager::GetSharedPlayerHealth()
{
	return sharedPlayerHealth;
}

bool GameManager::CanBeDamaged()
{
	return m_fInvincibilityTimer <= 0.f;
}

void GameManager::SharedPlayerHealthTakeDamage(int damage)
{
	sharedPlayerHealth = sharedPlayerHealth - damage;
	if (m_pHealthUI)
	{
		const float percentage = static_cast<float>(sharedPlayerHealth) / m_fMaxHealth;
		m_pHealthUI->SetHealthCullingEnd(percentage);
	}

	m_fInvincibilityTimer = m_fInvincibilityTime;
}

void GameManager::AddComboStreak(int comboNumber)
{
	comboStreak = comboStreak + comboNumber;
	if (m_pComboUI)
	{
		m_pComboUI->SetComboUI(std::to_string(comboStreak));
	}
}

void GameManager::AddToOnComboOnCatch(int comboNumber)
{
	comboOnCatch = comboOnCatch + comboNumber;
		if (Utils::InRangeExcluded(0, comboStreak, comboThreshold1) || comboStreak == 0)
		{
			if (m_pComboSlider)
			{
				m_pComboSlider->SetComboSlider(comboOnCatch, comboThreshold1);
			}
			if (m_pComboIcon)
			{
				m_pComboIcon->SetComboIconImage(0);
			}
		}
		if (Utils::InRangeExcluded(comboThreshold1, comboStreak, comboThreshold2))
		{
			if (m_pComboSlider)
			{
				m_pComboSlider->SetComboSlider(comboOnCatch, comboThreshold2);
			}
			if (m_pComboIcon)
			{
				m_pComboIcon->SetComboIconImage(1);
			}
		}
		if (Utils::InRangeExcluded(comboThreshold2, comboStreak, comboThreshold3))
		{
			if (m_pComboSlider)
			{
				m_pComboSlider->SetComboSlider(comboOnCatch, comboThreshold3);
			}
			if (m_pComboIcon)
			{
				m_pComboIcon->SetComboIconImage(2);
			}
		}
		if (comboStreak>comboThreshold3)
		{
			if (m_pComboSlider)
			{
				m_pComboSlider->SetComboSlider(comboOnCatch, comboThreshold3);
			}
			if (m_pComboIcon)
			{
				m_pComboIcon->SetComboIconImage(3);
			}
		}
}

void GameManager::ResetComboStreak()
{
	comboStreak = 0;
	if (m_pComboUI)
	{
		m_pComboUI->SetComboUI(std::to_string(comboStreak));
	}
	if (m_pComboSlider)
	{
		m_pComboSlider->SetComboSlider(comboStreak, comboStreak);
	}
	if (m_pComboIcon)
	{
		m_pComboIcon->SetComboIconImage(0);
	}
}