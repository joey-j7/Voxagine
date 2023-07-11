#include "pch.h"
#include "UIComponent.h"

#include "Core/ECS/World.h"

#include <External/rttr/registration.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "Core/ECS/Entities/UI/Canvas.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<UIComponent>("UIComponent")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Is Focusable", &UIComponent::GetIsFocusable, &UIComponent::SetIsFocusable) (RTTR_PUBLIC)
		.property("Default Focus", &UIComponent::GetDefaultFocus, &UIComponent::SetDefaultFocus) (RTTR_PUBLIC)
	
		.property("Navigation Next", &UIComponent::GetNextComponent, &UIComponent::SetNextComponent) (RTTR_PUBLIC)
		.property("Navigation Previous", &UIComponent::GetPreviousComponent, &UIComponent::SetPreviousComponent) (RTTR_PUBLIC)
		.property("Navigation Up", &UIComponent::GetUpComponent, &UIComponent::SetUpComponent) (RTTR_PUBLIC)
		.property("Navigation Down", &UIComponent::GetDownComponent, &UIComponent::SetDownComponent) (RTTR_PUBLIC)
		.property("Navigation Left", &UIComponent::GetLeftComponent, &UIComponent::SetLeftComponent) (RTTR_PUBLIC)
		.property("Navigation Right", &UIComponent::GetRightComponent, &UIComponent::SetRightComponent) (RTTR_PUBLIC)
	;
}

UIComponent::UIComponent(Entity * pOwner)
	: Component(pOwner)
{
}

UIComponent::~UIComponent()
{
}

void UIComponent::Awake()
{
	if (m_IsFocusable)
		RegisterToCanvas();

	m_bAwoken = true;
}

void UIComponent::Start()
{
	m_bStarted = true;
}

void UIComponent::RegisterToCanvas()
{
	Entity* parent = GetOwner();
	while (parent != nullptr)
	{
		if (Canvas* canvasOwner = dynamic_cast<Canvas*>(parent))
		{
			canvasOwner->RegisterUIComponent(this);
			m_Canvas = canvasOwner;
			break;
		}
		else
		{
			parent = parent->GetParent();
		}
	}
}


void UIComponent::SetIsFocusable(bool bFocusable)
{
	if (m_IsFocusable == bFocusable)
		return;

	m_IsFocusable = bFocusable;

	if (IsInitialized())
	{
		if (m_IsFocusable)
		{
			RegisterToCanvas();
		}
		else if(m_Canvas)
		{
			m_Canvas->RemoveUIComponent(this);
		}
	}		
}

bool UIComponent::GetIsFocusable() const
{
	return m_IsFocusable;
}

void UIComponent::SetDefaultFocus(bool bDefaultFocus)
{
	m_DefaultFocus = bDefaultFocus;
}

bool UIComponent::GetDefaultFocus() const
{
	return m_DefaultFocus;
}

void UIComponent::SetPreviousComponent(UIComponent * pComponent)
{
	m_PreviousComponent = pComponent;
}

UIComponent * UIComponent::GetPreviousComponent() const
{
	return m_PreviousComponent;
}

void UIComponent::SetNextComponent(UIComponent * pComponent)
{
	m_NextComponent = pComponent;
}

UIComponent * UIComponent::GetNextComponent() const
{
	return m_NextComponent;
}

void UIComponent::SetUpComponent(UIComponent * pComponent)
{
	m_UpComponent = pComponent;
}

UIComponent * UIComponent::GetUpComponent() const
{
	return m_UpComponent;
}

void UIComponent::SetDownComponent(UIComponent * pComponent)
{
	m_DownComponent = pComponent;
}

UIComponent * UIComponent::GetDownComponent() const
{
	return m_DownComponent;
}

void UIComponent::SetLeftComponent(UIComponent * pComponent)
{
	m_LeftComponent = pComponent;
}

UIComponent * UIComponent::GetLeftComponent() const
{
	return m_LeftComponent;
}

void UIComponent::SetRightComponent(UIComponent * pComponent)
{
	m_RightComponent = pComponent;
}

UIComponent * UIComponent::GetRightComponent() const
{
	return m_RightComponent;
}
