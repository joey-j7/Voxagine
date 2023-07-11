#include "LevelSelectCanvas.h"

#include "UI/WorldSwitch.h"

#include <External/glm/glm.hpp>

#include "Core/ECS/Components/SpriteRenderer.h"
#include "Core/ECS/World.h"
#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Input/Temp/InputContextNew.h"

#include "Core/PlayerPrefs/PlayerPrefs.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Core/Platform/Audio/AudioContext.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<LevelSelectWorld>("Level Select World")
		.constructor()(rttr::policy::ctor::as_object)
		.property("World", &LevelSelectWorld::sWorldFilePath) (RTTR_PUBLIC, RTTR_RESOURCE("wld"))
		.property("World Image", &LevelSelectWorld::sWorldImageFilePath) (RTTR_PUBLIC, RTTR_RESOURCE("png"))
	;

	rttr::registration::class_<LevelSelectCanvas>("LevelSelect Canvas")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Check If World Is Unlocked", &LevelSelectCanvas::m_bCheckForWorldUnlocked) (RTTR_PUBLIC)

		.property("Worlds", &LevelSelectCanvas::GetLevelSelectWorlds, &LevelSelectCanvas::SetLevelSelectWorlds) (RTTR_PUBLIC)
		.property("SpriteRenderer", &LevelSelectCanvas::m_pSpriteRenderer) (RTTR_PUBLIC)

		.property("Left Unlock World", &LevelSelectCanvas::m_pLeftUnlockWorldSpriteRenderer) (RTTR_PUBLIC)
		.property("Right Unlock World", &LevelSelectCanvas::m_pRightUnlockWorldSpriteRenderer) (RTTR_PUBLIC)

		.property("Movement Direction", &LevelSelectCanvas::m_MovementDirection) (RTTR_PUBLIC)
		.property("Open Offset", &LevelSelectCanvas::m_MovementOffset) (RTTR_PUBLIC)
		.property("Transition Duration", &LevelSelectCanvas::m_fMovementTransitionDuration) (RTTR_PUBLIC)

		.property("Loop Worlds", &LevelSelectCanvas::m_bLoopWorlds) (RTTR_PUBLIC)
	;
}

LevelSelectCanvas::LevelSelectCanvas(World* pWorld)
	: Canvas(pWorld)
{
}

LevelSelectCanvas::~LevelSelectCanvas()
{
}

void LevelSelectCanvas::Start()
{
	Canvas::Start();

	if (m_LevelSelectWorlds.size() > 0)
	{
		m_LevelSelectWorlds[0].bNewUnlocked = false;
		m_LevelSelectWorlds[0].bUnlocked = true;
	}

	for (size_t i = m_LevelSelectWorlds.size() - 1; i >= 1; i--) {
		m_LevelSelectWorlds[i].bUnlocked = PlayerPrefs::GetBoolAccessor().Get("Unlocked" + m_LevelSelectWorlds[i].sWorldFilePath, false);
		m_LevelSelectWorlds[i].bNewUnlocked = PlayerPrefs::GetBoolAccessor().Get("NewUnlocked" + m_LevelSelectWorlds[i].sWorldFilePath, false);
		if (m_LevelSelectWorlds[i].bNewUnlocked)
		{
			m_LevelSelectWorlds[i].bUnlocked = true;
			PlayerPrefs::GetBoolAccessor().Set("Unlocked" + m_LevelSelectWorlds[i].sWorldFilePath, m_LevelSelectWorlds[i].bUnlocked);
			PlayerPrefs::DeleteKey("NewUnlocked" + m_LevelSelectWorlds[i].sWorldFilePath);
			m_iCurrentlySelectedWorld = i;
		}
	}

	PlayerPrefs::Save();

	UpdateSelectedWorld();

	InputContextNew* pInputContext = GetWorld()->GetApplication()->GetPlatform().GetInputContext();
	if (pInputContext)
	{
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Back_UI", IKS_RELEASED, IK_B);
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Back_UI", IKS_RELEASED, IK_ESCAPE);
		pInputContext->RegisterAction(UI_INPUT_LAYER, "Back_UI", IKS_RELEASED, IK_GAMEPADRIGHTPADRIGHT);
		
		pInputContext->BindAction(UI_INPUT_LAYER, "Back_UI", IKS_RELEASED, m_InputBindings, BMT_PLAYERCONTROLLERS, [this]() {
			m_bGotoMain = true;
			GetWorld()->GetRenderSystem()->Fade();
		});
	}
}

void LevelSelectCanvas::Tick(float fDeltatime)
{
	Canvas::Tick(fDeltatime);

	if (m_bGotoMain)
	{
		if (GetWorld()->GetRenderSystem()->IsFaded())
		{
			WorldSwitch::SwitchWorld(GetWorld(), "Content/Worlds/Menus/Main_Menu.wld", false, false);
		}

		return;
	}
	else if (!m_sNextWorld.empty())
	{
		GetWorld()->GetApplication()->GetPlatform().GetAudioContext()->SetBGMVolume(
			GetWorld()->GetRenderSystem()->GetFadeValue() * 0.75f
		);

		if (GetWorld()->GetRenderSystem()->IsFaded())
		{
			GetWorld()->GetApplication()->GetPlatform().GetAudioContext()->StopBGM();
			GetWorld()->GetApplication()->GetPlatform().GetAudioContext()->SetBGMVolume(
				1.f
			);

			WorldSwitch::SwitchWorld(GetWorld(), m_sNextWorld, false, true);
		}
	}

	if (m_bOpening)
	{
		m_fCurrentMovementTransitionProgress += 1.f / m_fMovementTransitionDuration * fDeltatime;

		if (m_fCurrentMovementTransitionProgress >= 1.f)
			m_bOpening = false;

		m_fCurrentMovementTransitionProgress = glm::clamp(m_fCurrentMovementTransitionProgress, 0.f, 1.f);
		UpdateTransitionOffset(m_fCurrentMovementTransitionProgress);

		if (!m_bOpening)
			m_fCurrentMovementTransitionProgress = 0.f;
	}
	else if (m_fCurrentMovementTransitionProgress > 0.f)
	{
		m_fCurrentMovementTransitionProgress -= fDeltatime;
		if(m_fCurrentMovementTransitionProgress <= 0.f)
			m_bOpening = true;
	}
}

void LevelSelectCanvas::UpdateTransitionOffset(float progress)
{
	progress = glm::smoothstep(0.f, 1.f, progress);
	m_pLeftUnlockWorldSpriteRenderer->GetTransform()->SetLocalPosition({ m_MovementOffset * progress * -m_MovementDirection, m_pLeftUnlockWorldSpriteRenderer->GetTransform()->GetLocalPosition().z });
	m_pRightUnlockWorldSpriteRenderer->GetTransform()->SetLocalPosition({ m_MovementOffset * progress * m_MovementDirection, m_pRightUnlockWorldSpriteRenderer->GetTransform()->GetLocalPosition().z });
}


void LevelSelectCanvas::SetFocusLeft()
{
	Canvas::SetFocusLeft();

	m_iCurrentlySelectedWorld--;
	if (m_bLoopWorlds && m_iCurrentlySelectedWorld < 0)
		m_iCurrentlySelectedWorld = static_cast<int>(m_LevelSelectWorlds.size());

	UpdateSelectedWorld();
}

void LevelSelectCanvas::SetFocusRight()
{
	Canvas::SetFocusRight();

	m_iCurrentlySelectedWorld++;
	if (m_bLoopWorlds && m_iCurrentlySelectedWorld >= m_LevelSelectWorlds.size())
		m_iCurrentlySelectedWorld = 0;

	UpdateSelectedWorld();
}


void LevelSelectCanvas::OnReleased()
{
	Canvas::OnReleased();

	if (!m_sNextWorld.empty())
		return;

	// Check if unlocked.
	if (m_bCheckForWorldUnlocked && !GetCurrentLevelSelectWorld().bUnlocked && !GetCurrentLevelSelectWorld().bNewUnlocked)
		return;

	GetWorld()->GetRenderSystem()->Fade();
	m_sNextWorld = GetCurrentLevelSelectWorld().sWorldFilePath;
}

void LevelSelectCanvas::UpdateSelectedWorld()
{
	m_bOpening = false;
	m_fCurrentMovementTransitionProgress = 0.f;

	// Make sure the selected world is valid.
	m_iCurrentlySelectedWorld = glm::clamp(m_iCurrentlySelectedWorld, 0, static_cast<int>(m_LevelSelectWorlds.size()) - 1);

	if (m_pSpriteRenderer)
	{
		// Change world image
		m_pSpriteRenderer->SetFilePath(GetCurrentLevelSelectWorld().sWorldImageFilePath);

		if (GetCurrentLevelSelectWorld().bUnlocked && !GetCurrentLevelSelectWorld().bNewUnlocked)
		{
			// Opened
			UpdateTransitionOffset(1.f);
		}
		else 
		{
			// Closed
			UpdateTransitionOffset(0.f);
		}

		// Start opening
		if (GetCurrentLevelSelectWorld().bNewUnlocked)
		{
			m_fCurrentMovementTransitionProgress = .3f;
			GetCurrentLevelSelectWorld().bNewUnlocked = false;
		}
	}
}

void LevelSelectCanvas::SetLevelSelectWorlds(std::vector<LevelSelectWorld> levelSelectWorlds)
{
	if (!levelSelectWorlds.empty())
	{
		for (auto& type : levelSelectWorlds)
		{
			if (!type.GetOwner())
				type.SetOwner(this); // This is important
		}
	}

	m_LevelSelectWorlds = std::move(levelSelectWorlds);
}

