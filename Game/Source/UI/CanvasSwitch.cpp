#include "CanvasSwitch.h"

#include "Core/ECS/Components/UI/UIButton.h"
#include "Core/ECS/Entities/UI/Canvas.h"

#include "Core/ECS/World.h"
#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Input/Temp/InputContextNew.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<CanvasSwitch>("Canvas Switcher")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Button", &CanvasSwitch::m_pButton) (RTTR_PUBLIC)
		.property("Enable Back Button", &CanvasSwitch::m_bEnableBackButton) (RTTR_PUBLIC)

		.property("From Canvas", &CanvasSwitch::m_pFromCanvas) (RTTR_PUBLIC)
		.property("To Canvas", &CanvasSwitch::m_pToCanvas) (RTTR_PUBLIC)
	;
}

CanvasSwitch::CanvasSwitch(Entity* pEntity)
	: BehaviorScript(pEntity)
{
}

CanvasSwitch::~CanvasSwitch()
{
	InputContextNew* pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
	if (pInputContext)
		pInputContext->UnBindAction(m_InputBindings);
}

void CanvasSwitch::Start()
{
	if (!m_pButton)
	{
		// Try to get the button on this entity if there is none set.
		m_pButton = GetOwner()->GetComponent<UIButton>();
	}

	if(m_pButton && !m_pFromCanvas)
		m_pFromCanvas = m_pButton->GetCanvas();

	m_pButton->m_ClickedEvent += Event<UIButton*>::Subscriber([=](UIButton*)
	{
		// Check if the from canvas is actually enabled.
		if (m_pFromCanvas && m_pFromCanvas->IsEnabled() && m_pToCanvas)
		{
			m_pFromCanvas->SetEnabled(false);
			m_pToCanvas->SetEnabled(true);
		}
	}, this);

	// Register all input actions
	InputContextNew* pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
	if (pInputContext && m_bEnableBackButton)
	{
		// UI Back
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Back_UI", IKS_RELEASED, IK_B);
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Back_UI", IKS_RELEASED, IK_ESCAPE);
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Back_UI", IKS_RELEASED, IK_GAMEPADRIGHTPADRIGHT);

		pInputContext->BindAction(UI_INPUT_LAYER, "Back_UI", IKS_RELEASED, m_InputBindings, BMT_PLAYERCONTROLLERS, [&]() {
			// Check if the from canvas is actually enabled.
			if (m_pFromCanvas && m_pFromCanvas->IsEnabled() && m_bEnableBackButton && m_pToCanvas)
			{
				// Invoke delegate so the other scripts dont switch back.
				Invoke([&]() {
					m_pFromCanvas->SetEnabled(false);
					m_pToCanvas->SetEnabled(true);
				}, 0);
			}
		});
	}
}
