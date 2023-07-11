#include "pch.h"

#include "UISlider.h"

#include "Core/ECS/World.h"
#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Input/Temp/InputContextNew.h"

#include "Core/ECS/Components/SpriteRenderer.h"

#include "Core/ECS/Entities/UI/Canvas.h"

#include <External/glm/glm.hpp>

#include <External/rttr/registration.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::enumeration<EOrientation>("Orientation")
		(
			rttr::value("Horizontal", EOrientation::HORIZONTAL),
			rttr::value("Vertical",   EOrientation::VERTICAL)
		);

	rttr::registration::class_<UISlider>("UISlider")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)

		.property("Is ProgressBar", &UISlider::IsProgressBar, &UISlider::SetIsProgress) (RTTR_PUBLIC)

		.property("Value", &UISlider::GetValue, &UISlider::SetValue) (RTTR_PUBLIC)
		.property("StepSize", &UISlider::GetStepSize, &UISlider::SetStepSize) (RTTR_PUBLIC)
		.property("Step Interval MS", &UISlider::GetStepIntervalMS, &UISlider::SetStepIntervalMS) (RTTR_PUBLIC)
		.property("Orientation", &UISlider::GetOrientation, &UISlider::SetOrientation) (RTTR_PUBLIC)
		.property("Inverted", &UISlider::GetInverted, &UISlider::SetInverted) (RTTR_PUBLIC)

		.property("Slider Background", &UISlider::m_SliderBackground) (RTTR_PUBLIC)
		.property("Slider Bar", &UISlider::m_SliderBar) (RTTR_PUBLIC)
		.property("Slider Handle", &UISlider::m_SliderHandleDefault) (RTTR_PUBLIC)
		.property("Slider Handle Focused", &UISlider::m_SliderHandleFocused) (RTTR_PUBLIC)
		.property("Slider Handle Disabled", &UISlider::m_SliderHandleDisabled) (RTTR_PUBLIC)
	;
}

UISlider::UISlider(Entity * pOwner)
	: UIComponent(pOwner)
	, m_bProgressBar(false)
	, m_Value(0.f)
	, m_StepSize(0.005f)
	, m_StepInterval(.030f) // 30 ms
	, m_Orientation(EOrientation::HORIZONTAL)
	, m_bInverted(false)
{
}

UISlider::~UISlider()
{
	RemoveInputBindings();
}

void UISlider::Awake()
{
	UIComponent::Awake();
}
void UISlider::Start()
{
	UIComponent::Start();

	CreateInputBindings();
	UpdateSliderRepresentation();

	SetIsFocusable(GetIsFocusable());
}

void UISlider::SetIsFocusable(bool bFocusable)
{
	UIComponent::SetIsFocusable(bFocusable);

	if (bFocusable) {
		SetState(EUIState::DEFAULT);
	}
	else {
		SetState(EUIState::DISABLED);
	}
}

void UISlider::UpdateSliderRepresentation()
{
	if (!m_bAwoken)
		return;

	if (m_SliderBackground == nullptr)
		return;

	Vector2 size = static_cast<Vector2>(m_SliderBackground->GetSize()) * Vector2(m_SliderBackground->GetTransform()->GetScale());
	Vector2 pos = size * GetValue();
	if (m_bInverted)
		pos = size - pos;

	Vector3 finalPosition = Vector3(0.f);

	Vector2 cullStart = Vector2(0.f);
	Vector2 cullEnd = Vector2(1.f);

	if (m_Orientation == EOrientation::HORIZONTAL)
	{
		finalPosition = Vector3(pos.x / m_SliderBackground->GetScale().x, 0.f, 0.f);

		if (m_bInverted) {
			cullStart.x = GetValue();
		}
		else {
			cullEnd.x = GetValue();
		}
	}
	else if (m_Orientation == EOrientation::VERTICAL)
	{
		finalPosition = Vector3(0.f, pos.y / m_SliderBackground->GetScale().y, 0.f);

		if (m_bInverted) {
			cullStart.y = GetValue();
		}
		else {
			cullEnd.y = GetValue();
		}
	}

	if (m_SliderBar)
	{
		m_SliderBar->SetCullingStart(cullStart);
		m_SliderBar->SetCullingEnd(cullEnd);
	}

	if (!IsProgressBar())
	{
		if (m_SliderHandleDefault)
			m_SliderHandleDefault->GetTransform()->SetLocalPosition(finalPosition);
		if (m_SliderHandleFocused)
			m_SliderHandleFocused->GetTransform()->SetLocalPosition(finalPosition);
		if (m_SliderHandleDisabled)
			m_SliderHandleDisabled->GetTransform()->SetLocalPosition(finalPosition);
	}
}

void UISlider::CreateInputBindings()
{
	if (IsProgressBar())
		return;

	// Remove existing bindings if set.
	if (m_InputBindings.size() > 0)
		RemoveInputBindings();

	// Register all input actions
	InputContextNew* pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
	if (pInputContext)
	{
		//// Register actions once.
		//static bool s_bRegisteredSliderActions = false;
		//if (!s_bRegisteredSliderActions)
		//{
			pInputContext->CreateBindingMap(UI_INPUT_LAYER, false);

			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Horizontal_Increment_UI", IKS_HELD, IK_RIGHT);
			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Horizontal_Increment_UI", IKS_HELD, IK_D);
			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Horizontal_Increment_UI", IKS_HELD, IK_GAMEPADLEFTPADRIGHT);

			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Horizontal_Decrement_UI", IKS_HELD, IK_LEFT);
			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Horizontal_Decrement_UI", IKS_HELD, IK_A);
			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Horizontal_Decrement_UI", IKS_HELD, IK_GAMEPADLEFTPADLEFT);

			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Vertical_Increment_UI", IKS_HELD, IK_UP);
			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Vertical_Increment_UI", IKS_HELD, IK_W);
			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Vertical_Increment_UI", IKS_HELD, IK_GAMEPADLEFTPADUP);

			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Vertical_Decrement_UI", IKS_HELD, IK_DOWN);
			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Vertical_Decrement_UI", IKS_HELD, IK_S);
			pInputContext->RegisterAction(UI_INPUT_LAYER, "Slider_Vertical_Decrement_UI", IKS_HELD, IK_GAMEPADLEFTPADDOWN);

			//s_bRegisteredSliderActions = true;
		//}

		// Bind the callbacks to the actions.
		if (m_Orientation == EOrientation::HORIZONTAL)
		{
			pInputContext->BindAction(UI_INPUT_LAYER, "Slider_Horizontal_Increment_UI", IKS_HELD, m_InputBindings, BMT_PLAYERCONTROLLERS, std::bind(&UISlider::UpdateValueChange, this, true));
			pInputContext->BindAction(UI_INPUT_LAYER, "Slider_Horizontal_Decrement_UI", IKS_HELD, m_InputBindings, BMT_PLAYERCONTROLLERS, std::bind(&UISlider::UpdateValueChange, this, false));
		}
		else if (m_Orientation == EOrientation::VERTICAL)
		{
			pInputContext->BindAction(UI_INPUT_LAYER, "Slider_Vertical_Increment_UI", IKS_HELD, m_InputBindings, BMT_PLAYERCONTROLLERS, std::bind(&UISlider::UpdateValueChange, this, true));
			pInputContext->BindAction(UI_INPUT_LAYER, "Slider_Vertical_Decrement_UI", IKS_HELD, m_InputBindings, BMT_PLAYERCONTROLLERS, std::bind(&UISlider::UpdateValueChange, this, false));
		}
	}
}
void UISlider::RemoveInputBindings()
{
	if (m_InputBindings.size() <= 0)
		return;

	if(GetWorld()->GetApplication()->GetPlatform().GetInputContext())
		GetWorld()->GetApplication()->GetPlatform().GetInputContext()->UnBindAction(m_InputBindings);

	m_InputBindings.clear();
}

void UISlider::UpdateValueChange(bool bIncrement)
{
	if (!m_Focused || !GetIsFocusable() || IsProgressBar())
		return;

	if (GetWorld()->GetTotalSeconds() - m_PreviousTime > GetStepInterval())
	{
		// Invert input
		if (m_bInverted)
			bIncrement = !bIncrement;

		if (bIncrement)
		{
			IncrementValue();
		}
		else
		{
			DecrementValue();
		}

		m_PreviousTime = GetWorld()->GetTotalSeconds();
	}
}
void UISlider::SetValue(float fValue)
{
	if (m_Value != fValue)
	{
		m_Value = glm::clamp(fValue, 0.f, 1.f);

		m_ValueChangedEvent(this, m_Value);

		UpdateSliderRepresentation();
	}
}
void UISlider::IncrementValue()
{
	SetValue(GetValue() + GetStepSize());
}
void UISlider::DecrementValue()
{
	SetValue(GetValue() - GetStepSize());
}
float UISlider::GetValue() const
{
	return m_Value;
}

void UISlider::SetStepSize(float fStepSize)
{
	m_StepSize = glm::clamp(fStepSize, 0.f, 1.f);
}
float UISlider::GetStepSize() const
{
	return m_StepSize;
}

void UISlider::SetStepInterval(float fStepInterval)
{
	m_StepInterval = fStepInterval;
}
float UISlider::GetStepInterval() const
{
	return m_StepInterval;
}
void UISlider::SetStepIntervalMS(float fStepIntervalMS)
{
	m_StepInterval = fStepIntervalMS * 0.001f;
}
float UISlider::GetStepIntervalMS() const
{
	return m_StepInterval * 1000.f;
}

void UISlider::SetOrientation(EOrientation Orientation)
{
	if (m_Orientation == Orientation)
		return;

	m_Orientation = Orientation;

	if (m_SliderBackground)
		m_SliderBackground->SetAlignment(m_Orientation == EOrientation::HORIZONTAL ? RenderAlignment::RA_LEFTCENTER : RenderAlignment::RA_BOTTOMCENTER);

	CreateInputBindings();
	UpdateSliderRepresentation();
}
EOrientation UISlider::GetOrientation() const
{
	return m_Orientation;
}

void UISlider::SetInverted(bool bInverted)
{
	if (m_bInverted == bInverted)
		return;

	m_bInverted = bInverted;

	CreateInputBindings();
	UpdateSliderRepresentation();
}
bool UISlider::GetInverted() const
{
	return m_bInverted;
}

void UISlider::SetIsProgress(bool bIsProgress)
{
	m_bProgressBar = bIsProgress;

	SetIsFocusable(GetIsFocusable());

	if (IsProgressBar())
		SetIsFocusable(false);
}

bool UISlider::IsProgressBar() const
{
	return m_bProgressBar;
}


void UISlider::OnFocus()
{
	UIComponent::OnFocus();

	if (!GetIsFocusable())
		return;

	m_FocusEvent(this);

	SetState(EUIState::HOVERED);
}
void UISlider::OnFocusLost()
{
	UIComponent::OnFocusLost();
	
	if (!GetIsFocusable())
		return;

	m_LostFocusEvent(this);

	SetState(EUIState::DEFAULT);
}

void UISlider::SetState(EUIState eUIState)
{
	if (m_SliderHandleDefault)
		m_SliderHandleDefault->SetEnabled(eUIState == EUIState::DEFAULT && !IsProgressBar());
	if (m_SliderHandleFocused)
		m_SliderHandleFocused->SetEnabled(eUIState == EUIState::HOVERED && !IsProgressBar());
	if (m_SliderHandleDisabled)
		m_SliderHandleDisabled->SetEnabled(eUIState == EUIState::DISABLED && !IsProgressBar());
}
