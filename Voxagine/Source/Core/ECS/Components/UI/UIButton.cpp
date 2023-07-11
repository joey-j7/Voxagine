#include "pch.h"
#include "UIButton.h"

#include "Core/ECS/World.h"

#include <External/rttr/registration.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "../../Entity.h"
#include "../AudioSource.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<UIButton>("UIButton")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Normal Object", &UIButton::m_NormalObject) (RTTR_PUBLIC)
		.property("Focused Object", &UIButton::m_FocusedObject) (RTTR_PUBLIC)
		.property("Pressed Object", &UIButton::m_PressedObject) (RTTR_PUBLIC)
		.property("Disabled Object", &UIButton::m_DisabledObject) (RTTR_PUBLIC);
}

UIButton::UIButton(Entity * pOwner)
	: UIComponent(pOwner)
{
}

UIButton::~UIButton()
{
}

void UIButton::Awake()
{
	UIComponent::Awake();

	SetIsFocusable(GetIsFocusable());
}

void UIButton::Start()
{
	m_pAudioSource = GetOwner()->GetComponent<AudioSource>();
	if (!m_pAudioSource) m_pAudioSource = GetOwner()->AddComponent<AudioSource>();

	m_pAudioSource->SetLooping(false);
	m_pAudioSource->Set3DAudio(false);
}


void UIButton::SetIsFocusable(bool bInteractable)
{
	if (bInteractable == GetIsFocusable())
		return;

	if (bInteractable)
	{
		SetState(EUIState::DEFAULT);
	}
	else
	{
		SetState(EUIState::DISABLED);
	}

	UIComponent::SetIsFocusable(bInteractable);
}


void UIButton::OnFocus()
{
	UIComponent::OnFocus();

	if (!GetIsFocusable())
		return;

	SetState(EUIState::HOVERED);

	m_FocusEvent(this);

	if (m_pAudioSource) 
	{
		// Focus audio
		m_pAudioSource->SetLooping(false);

		m_pAudioSource->SetFilePath("Content/SFX/Menu/ButtonNavigation.ogg");
		m_pAudioSource->Play();
	}
}

void UIButton::OnFocusLost()
{
	UIComponent::OnFocusLost();

	if (!GetIsFocusable())
		return;

	SetState(EUIState::DEFAULT);

	m_LostFocusEvent(this);
}

void UIButton::OnPressed()
{
	UIComponent::OnPressed();

	if (!GetIsFocusable())
		return;

	SetState(EUIState::PRESSED);

	m_PressedEvent(this);

	if (m_pAudioSource)
	{
		// Focus audio
		m_pAudioSource->SetLooping(false);

		m_pAudioSource->SetFilePath("Content/SFX/Menu/ButtonClick.ogg");
		m_pAudioSource->Play();
	}
}

void UIButton::OnReleased()
{
	UIComponent::OnReleased();

	if (!GetIsFocusable())
		return;

	SetState(EUIState::HOVERED);

	m_ReleasedEvent(this);
}

void UIButton::OnClicked()
{
	UIComponent::OnClicked();

	if (!GetIsFocusable())
		return;

	m_ClickedEvent(this);
}

void UIButton::SetState(EUIState eUIState)
{
	if (m_NormalObject)
		m_NormalObject->SetEnabled(eUIState == EUIState::DEFAULT);
	if (m_FocusedObject)
		m_FocusedObject->SetEnabled(eUIState == EUIState::HOVERED);
	if (m_PressedObject)
		m_PressedObject->SetEnabled(eUIState == EUIState::PRESSED);
	if (m_DisabledObject)
		m_DisabledObject->SetEnabled(eUIState == EUIState::DISABLED);
}
