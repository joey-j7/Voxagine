#include "StartToJoinPlayerComponent.h"

#include "Humanoids/Players/Player.h"
#include "Core/ECS/Components/InputHandler.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"

#include "Core/ECS/Entities/UI/Canvas.h"
#include "Core/ECS/Components/VoxAnimator.h"

#include "Core/ECS/Components/AudioSource.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
RTTR_REGISTRATION
{
	rttr::registration::class_<StartToJoinPlayerComponent>("StartToJoinPlayerComponent")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Press Start To Join Visuals", &StartToJoinPlayerComponent::m_pStartToJoinVisuals)(RTTR_PUBLIC)
		.property("Join Visual Interval", &StartToJoinPlayerComponent::m_fFlickerInterval)(RTTR_PUBLIC)

		.property("Player Joined Visuals", &StartToJoinPlayerComponent::m_pJoinedVisuals)(RTTR_PUBLIC)

		.property("Assigned Player", &StartToJoinPlayerComponent::m_pPlayer)(RTTR_PUBLIC)

		.property("Error Interval", &StartToJoinPlayerComponent::m_fPlayerErrorInterval)(RTTR_PUBLIC)
		.property("Error Switch Times", &StartToJoinPlayerComponent::m_iPlayerErrorSwitchTimes)(RTTR_PUBLIC)

		.property("Default Anim Index", &StartToJoinPlayerComponent::m_iDefaultPlayerAnimIndex)(RTTR_PUBLIC)
		.property("Disabled Anim Index", &StartToJoinPlayerComponent::m_iDisabledPlayerAnimIndex)(RTTR_PUBLIC)
		.property("Error Anim Index", &StartToJoinPlayerComponent::m_iErrorPlayerAnimIndex)(RTTR_PUBLIC)

	;
}

StartToJoinPlayerComponent::StartToJoinPlayerComponent(Entity* m_pEntity)
	: BehaviorScript(m_pEntity)
{
}

StartToJoinPlayerComponent::~StartToJoinPlayerComponent()
{
}

void StartToJoinPlayerComponent::Awake()
{
	// Register all input actions
	InputContextNew* pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
	if (pInputContext)
	{
		// Create binding map and register actions to make sure they exist. Inside they check if its already created or registered/
		pInputContext->CreateBindingMap(UI_INPUT_LAYER, false);

		pInputContext->RegisterAction(UI_INPUT_LAYER, "MainMenu_PressToJoin", IKS_PRESSED, IK_GAMEPADOPTION);
		pInputContext->RegisterAction(UI_INPUT_LAYER, "MainMenu_PressToJoin", IKS_PRESSED, IK_MOUSEBUTTONLEFT);
	}
}

void StartToJoinPlayerComponent::Start()
{
	if (m_pPlayer)
	{
		InputHandler* m_pInputHandler = m_pPlayer->GetComponent<InputHandler>();

		m_pInputHandler->BindAction(UI_INPUT_LAYER, "MainMenu_PressToJoin", IKS_PRESSED, std::bind(&StartToJoinPlayerComponent::TogglePlayerShowing, this));
	}

	if (m_pPlayer)
		m_pPlayerRenderer = m_pPlayer->GetComponent<VoxAnimator>();


	SetJoinVisualActive(m_iCurrentStartToJoinVisual);
	SetPlayerAnimIndex(m_iDisabledPlayerAnimIndex);

	m_pAudioSource = GetOwner()->GetComponent<AudioSource>();
	if (!m_pAudioSource)
		m_pAudioSource = GetOwner()->AddComponent<AudioSource>();

	m_pAudioSource->SetFilePath("Content/SFX/Menu/ButtonError.ogg");
	m_pAudioSource->SetLooping(false);
	m_pAudioSource->Set3DAudio(false);
}


void StartToJoinPlayerComponent::Tick(float fDeltaTime)
{
	m_fProgressIntervalTime += fDeltaTime;
	if (m_fProgressIntervalTime > m_fFlickerInterval)
	{
		m_fProgressIntervalTime = 0.f;

		// Increment join visual
		m_iCurrentStartToJoinVisual++;
		if (m_iCurrentStartToJoinVisual >= m_pStartToJoinVisuals.size())
			m_iCurrentStartToJoinVisual = 0;
	}

	if (m_bShowErrorAnim && !PlayerJoined())
	{
		m_fPlayerErrorProgress += fDeltaTime;
		if (m_fPlayerErrorProgress > m_fPlayerErrorInterval) 
		{
			m_fPlayerErrorProgress = 0.f;
			m_iPlayerErrorCurrentSwitchTimes++;

			m_bShowErrorVisual = !m_bShowErrorVisual;
			SetPlayerAnimIndex(m_bShowErrorVisual ? m_iErrorPlayerAnimIndex : m_iDisabledPlayerAnimIndex);
			
			if (m_iPlayerErrorCurrentSwitchTimes >= (m_iPlayerErrorSwitchTimes * 2) + 1)
			{
				m_bShowErrorAnim = false;
			}
		}
	}

	if (!PlayerJoined())
	{
		// Update visuals
		SetJoinVisualActive(m_iCurrentStartToJoinVisual);
	}
}

void StartToJoinPlayerComponent::ShowErrorAnim()
{
	if (PlayerJoined())
		return;

	m_pAudioSource->Play();

	m_bShowErrorAnim = true;
	m_bShowErrorVisual = true;
	m_fPlayerErrorProgress = 0.f;
	m_iPlayerErrorCurrentSwitchTimes = 0;
}

void StartToJoinPlayerComponent::TogglePlayerShowing()
{
	m_bPlayerJoined = !m_bPlayerJoined;

	if (PlayerJoined())
	{
		SetJoinVisualActive(-1);
	
		SetPlayerAnimIndex(m_iDefaultPlayerAnimIndex);
	}
	else 
	{
		SetJoinVisualActive(m_iCurrentStartToJoinVisual);

		SetPlayerAnimIndex(m_iDisabledPlayerAnimIndex);
	}
}

void StartToJoinPlayerComponent::SetJoinVisualActive(int index)
{
	for (Entity* entity : m_pJoinedVisuals)
		entity->SetEnabled(index < 0);
	
	for (size_t i = 0; i < m_pStartToJoinVisuals.size(); i++)
		m_pStartToJoinVisuals[i]->SetEnabled(i == index);
}

void StartToJoinPlayerComponent::SetPlayerAnimIndex(unsigned int animIndex)
{
	if (m_pPlayerRenderer)
	{
		uint32_t currentFrame = m_pPlayerRenderer->GetCurrentFrameIndex();
		m_pPlayerRenderer->SetCurrentAnimationWithFrame(animIndex, currentFrame);
	}
}
