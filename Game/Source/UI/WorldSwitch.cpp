#include "WorldSwitch.h"

#include "Core/ECS/Components/UI/UIButton.h"
#include "Core/ECS/World.h"

#include "Core/Application.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Input/Temp/InputContextNew.h"
#include <Core/ECS/Entities/UI/Canvas.h>
#include <Core/Platform/Audio/AudioContext.h>

#define LOADING_SCREEN_WORLD_FILEPATH "Content/Worlds/Menus/Loadingscreen.wld"

RTTR_REGISTRATION
{
	rttr::registration::class_<WorldSwitch>("World Switch")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Button", &WorldSwitch::m_pButton) (RTTR_PUBLIC)
		.property("World To Load", &WorldSwitch::m_sWorldFilename) (RTTR_PUBLIC, RTTR_RESOURCE("wld"))
		.property("Load Previous World", &WorldSwitch::m_bLoadPreviousWorld) (RTTR_PUBLIC)

		.property("Load World Async", &WorldSwitch::m_bAsync) (RTTR_PUBLIC)
		.property("Use Loading Screen", &WorldSwitch::m_bUseLoadingScreen) (RTTR_PUBLIC)

		.property("Fade out BGM", &WorldSwitch::m_bFadeBGM) (RTTR_PUBLIC)
		.property("World to pop", &WorldSwitch::m_uiWorldToPop) (RTTR_PUBLIC)
	;
}

WorldSwitch::WorldSwitch(Entity* pEntity)
	: BehaviorScript(pEntity)
{
}

WorldSwitch::~WorldSwitch()
{
}

void WorldSwitch::Start()
{
	if (!m_pButton)
	{
		// Try to get the button on this entity if there is none set.
		m_pButton = GetOwner()->GetComponent<UIButton>();
	}

	m_pButton->m_ClickedEvent += Event<UIButton*>::Subscriber([=](UIButton*)
	{
		if (m_bClicked)
			return;

		if (m_bLoadPreviousWorld)
		{
			m_sWorldFilename = GetWorld()->GetApplication()->GetWorldManager().GetPreviousWorldName();
			printf("Previous World: %s", m_sWorldFilename.c_str());
		}

		m_bClicked = true;
		GetWorld()->GetRenderSystem()->Fade();

		m_fBGMVolume = GetWorld()->GetApplication()->GetPlatform().GetAudioContext()->GetBGMVolume();

		GetWorld()->GetApplication()->GetPlatform().GetInputContext()->CreateBindingMap("Disabled", true);
	}, this);
}


void WorldSwitch::Tick(float fDeltaTime)
{
	if (m_bClicked)
	{
		if (GetWorld()->GetRenderSystem()->IsFaded())
		{
			GetWorld()->GetApplication()->GetPlatform().GetAudioContext()->StopBGM();

			GetWorld()->GetApplication()->GetPlatform().GetAudioContext()->SetBGMVolume(
				0.75f
			);

			m_bClicked = false;

			for (uint32_t i = 0; i < m_uiWorldToPop; ++i)
			{
				GetWorld()->GetApplication()->GetWorldManager().PopWorld();
			}

			SwitchWorld(GetWorld(), m_sWorldFilename, m_bAsync, m_bUseLoadingScreen);
		}
		else
		{
			GetWorld()->GetApplication()->GetPlatform().GetAudioContext()->SetBGMVolume(
				m_fBGMVolume * GetWorld()->GetRenderSystem()->GetFadeValue()
			);
		}
	}
}

static void LoadNextWorld(World* m_pNewWorld, std::string m_sWorldFilepath, bool m_bLoadAsync, bool m_bUseLoadingScreen = false, World* m_pPreviousWorld = nullptr)
{
	if (m_bUseLoadingScreen)
	{
		m_pNewWorld->GetApplication()->GetWorldManager().WorldLoaded -= m_pPreviousWorld;
		m_pNewWorld->GetRenderSystem()->SetFadeValue(1.f);
	}

	// Open the world and switch to it
	if (m_bLoadAsync || m_bUseLoadingScreen) {
		m_pNewWorld->OpenWorldAsync(m_sWorldFilepath);
	}
	else {
		m_pNewWorld->OpenWorld(m_sWorldFilepath);
	}
}

void WorldSwitch::SwitchWorld(World* m_pWorld, std::string m_sWorldFilepath, bool m_bLoadAsync, bool m_bUseLoadingScreen)
{
	// Load the new world
	if (m_sWorldFilepath == "")
		m_pWorld->GetApplication()->Exit();
	
	if (m_bUseLoadingScreen)
	{
		m_pWorld->GetApplication()->GetWorldManager().WorldLoaded += Event<World*>::Subscriber(std::bind(&LoadNextWorld, std::placeholders::_1, m_sWorldFilepath, m_bLoadAsync, m_bUseLoadingScreen, m_pWorld), m_pWorld);

		// Load the loading world
		if (m_bLoadAsync) {
			m_pWorld->OpenWorldAsync(LOADING_SCREEN_WORLD_FILEPATH);
		}
		else {
			m_pWorld->OpenWorld(LOADING_SCREEN_WORLD_FILEPATH);
		}
	}
	else
	{
		LoadNextWorld(m_pWorld, m_sWorldFilepath, m_bLoadAsync);
	}
}
