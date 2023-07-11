#include "SplashScreenHandler.h"

#include "UI/WorldSwitch.h"

#include "Core/ECS/Components/SpriteRenderer.h"

#include <External\glm\gtx\compatibility.hpp>

#include "Core/ECS/World.h"
#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Input/Temp/InputContextNew.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<SplashScreenHandler>("SplashScreen Handler")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Show Duration Seconds", &SplashScreenHandler::m_fWorldChangeTime) (RTTR_PUBLIC)
		.property("Main Menu World", &SplashScreenHandler::m_sMainMenuWorld) (RTTR_PUBLIC, RTTR_RESOURCE("wld"))

		.property("Splash Screen Sprites", &SplashScreenHandler::m_pSpriteRenderers) (RTTR_PUBLIC)
		.property("Time To Start Fade", &SplashScreenHandler::m_fStartTime) (RTTR_PUBLIC)
		.property("Fade Duration", &SplashScreenHandler::m_fFadeDuration) (RTTR_PUBLIC)
		.property("Splash Screen Show Duration Seconds", &SplashScreenHandler::m_fShowDuration) (RTTR_PUBLIC)
	;
}

SplashScreenHandler::SplashScreenHandler(Entity* pEntity)
	: BehaviorScript(pEntity)
{
}

SplashScreenHandler::~SplashScreenHandler()
{
	GetWorld()->GetApplication()->GetPlatform().GetInputContext()->UnBindAction(m_InputBindings);
}

void SplashScreenHandler::Start()
{
	if (m_sMainMenuWorld.size() > 0)
	{
		Invoke([&]() {
			WorldSwitch::SwitchWorld(GetWorld(), m_sMainMenuWorld, false, false);
		}, m_fWorldChangeTime);

		InputContextNew* pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
		if (pInputContext)
		{
			// Released UI
			pInputContext->RegisterAction("Skip_UI", IKS_RELEASED, IK_ENTER);
			pInputContext->RegisterAction("Skip_UI", IKS_RELEASED, IK_ESCAPE);
			pInputContext->RegisterAction("Skip_UI", IKS_RELEASED, IK_SPACE);
			pInputContext->RegisterAction("Skip_UI", IKS_RELEASED, IK_GAMEPADSELECT);
			pInputContext->RegisterAction("Skip_UI", IKS_RELEASED, IK_GAMEPADRIGHTPADDOWN);

			pInputContext->BindAction("Skip_UI", IKS_RELEASED, m_InputBindings, BMT_PLAYERCONTROLLERS, [&]() {
				WorldSwitch::SwitchWorld(GetWorld(), m_sMainMenuWorld, false, false);
			});
		}
	}

	for (auto pSpriteRenderer : m_pSpriteRenderers)
	{
		if (pSpriteRenderer)
		{
			Vector4 color = pSpriteRenderer->GetColor().GetVector4();
			color.a = 0.f;
			pSpriteRenderer->SetColor(color);
		}
	}
	
	Invoke([&]() {
		m_fCurrentProgress = 0.f;
		m_fFadeDirection = 1.f;
	}, m_fStartTime);
}

void SplashScreenHandler::Tick(float fDeltaTime)
{
	if (m_fFadeDirection == 0.f)
		return;

	if (m_pSpriteRenderers.size() <= 0)
		return;

	m_fCurrentProgress += (1.f / m_fFadeDuration) * fDeltaTime;

	Vector4 color = m_pSpriteRenderers[0]->GetColor().GetVector4();
	if (m_fFadeDirection < 0)
	{
		color.a = glm::smoothstep(1.f, 0.f, m_fCurrentProgress);
	}
	else if (m_fFadeDirection > 0)
	{
		color.a = glm::smoothstep(0.f, 1.f, m_fCurrentProgress);
	}
	
	if (color.a >= 1.f && m_fFadeDirection > 0.f) {
		m_fFadeDirection = 0.f;
		Invoke([&]() {
			m_fCurrentProgress = 0.f;
			m_fFadeDirection = -1.f;
		}, m_fShowDuration);
	}
	else if (color.a <= 0.f && m_fFadeDirection < 0.f) {
		m_fFadeDirection = 0.f;
	}

	color.a = glm::clamp(color.a, 0.f, 1.f);

	for (auto pSpriteRenderer : m_pSpriteRenderers)
		pSpriteRenderer->SetColor(color);
}
