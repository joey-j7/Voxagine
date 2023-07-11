#include "PauseScreenHandler.h"

#include "Core/ECS/Entities/UI/Canvas.h"
#include "Core/ECS/Components/UI/UIButton.h"

#include "Core/ECS/World.h"
#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Input/Temp/InputContextNew.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<PauseScreenHandler>("Pause Screen")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Paused Screen Canvas", &PauseScreenHandler::m_pPausedCanvas) (RTTR_PUBLIC)
		.property("Resume Buttons", &PauseScreenHandler::m_pResumeButtons) (RTTR_PUBLIC)
	;
}

PauseScreenHandler::PauseScreenHandler(Entity* pEntity)
	: BehaviorScript(pEntity)
{
}

PauseScreenHandler::~PauseScreenHandler()
{
	InputContextNew* pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
	if (pInputContext)
	{
		pInputContext->UnBindAction(m_InputBindings);
	}
}

void PauseScreenHandler::Awake()
{
	Component::Awake();

	// Register all input actions
	InputContextNew* pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
	if (pInputContext)
	{
		pInputContext->CreateBindingMap(UI_INPUT_LAYER, false);

		pInputContext->RegisterAction(UI_INPUT_LAYER, "Paused_UI", IKS_PRESSED, IK_PAUSE);
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Paused_UI", IKS_PRESSED, IK_ESCAPE);
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Paused_UI", IKS_PRESSED, IK_GAMEPADOPTION);
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Paused_UI", IKS_PRESSED, IK_GAMEPADSELECT);

		// Bind callback action tot the registered actions.
		pInputContext->BindAction(UI_INPUT_LAYER, "Paused_UI",
			IKS_PRESSED,
			m_InputBindings,
			BMT_PLAYERCONTROLLERS,
			[&]()
		{
			m_bResumed = true;
			GetWorld()->GetRenderSystem()->Fade();
			GetWorld()->GetApplication()->GetPlatform().GetInputContext()->CreateBindingMap("Disabled", true);
		});
	}
}

void PauseScreenHandler::Start()
{
	for (auto& pResumeButton : m_pResumeButtons)
	{
		pResumeButton->m_ClickedEvent += Event<UIButton*>::Subscriber([=](UIButton*)
		{
			m_bResumed = true;
			GetWorld()->GetRenderSystem()->Fade();
			GetWorld()->GetApplication()->GetPlatform().GetInputContext()->CreateBindingMap("Disabled", true);
		}, this);
	}

	m_pPausedCanvas->SetEnabled(true);
}

void PauseScreenHandler::Tick(float fDeltaTime)
{
	if (m_bResumed && GetWorld()->GetRenderSystem()->IsFaded())
	{
		ResumeGame();
	}
}

void PauseScreenHandler::ResumeGame()
{
	// Pop the pause world to resume the game
	GetWorld()->GetApplication()->GetWorldManager().PopWorld();
}

