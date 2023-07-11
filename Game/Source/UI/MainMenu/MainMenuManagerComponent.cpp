#include "MainMenuManagerComponent.h"

#include "./StartToJoinPlayerComponent.h"
#include "Core/ECS/Components/UI/UIButton.h"

#include "UI/WorldSwitch.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
RTTR_REGISTRATION
{
	rttr::registration::class_<MainMenuManagerComponent>("Main Menu Manager")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Start To Join Objects", &MainMenuManagerComponent::m_pStartToJoinComponents)(RTTR_PUBLIC)
		.property("Entities To DeActivate On Players Joined", &MainMenuManagerComponent::m_pDeactivateEntitiesOnPlayersJoined)(RTTR_PUBLIC)

		.property("Level Selection Button", &MainMenuManagerComponent::m_pLevelSelectButton)(RTTR_PUBLIC)
		.property("Level Selection World", &MainMenuManagerComponent::m_sLevelSelectionWorld)(RTTR_PUBLIC, RTTR_RESOURCE("wld"))
	;
}

uint32_t MainMenuManagerComponent::m_uiPlayerCount = 0;

bool MainMenuManagerComponent::m_bPlayer1Active = false;
bool MainMenuManagerComponent::m_bPlayer2Active = false;

MainMenuManagerComponent::MainMenuManagerComponent(Entity* m_pEntity)
	: BehaviorScript(m_pEntity)
{
}

MainMenuManagerComponent::~MainMenuManagerComponent()
{
}

void MainMenuManagerComponent::Start()
{
	BehaviorScript::Start();

	m_uiPlayerCount = 0;

	m_bPlayer1Active = false;
	m_bPlayer2Active = false;

	if (m_pLevelSelectButton)
	{
		m_pLevelSelectButton->m_ClickedEvent += Event<UIButton*>::Subscriber([=](UIButton*)
		{
			if (m_uiPrevPlayerCount > 0)
			{
				m_uiPlayerCount = m_uiPrevPlayerCount;

				GetWorld()->GetRenderSystem()->Fade();
				m_bGotoLevelSelection = true;
			}
			else
			{
				for (auto& pStartToJoin : m_pStartToJoinComponents)
					pStartToJoin->ShowErrorAnim();
			}
		}, this);
	}
}

void MainMenuManagerComponent::Tick(float fDeltaTime)
{
	BehaviorScript::Tick(fDeltaTime);

	if (m_bGotoLevelSelection)
	{
		if (GetWorld()->GetRenderSystem()->IsFaded())
			WorldSwitch::SwitchWorld(GetWorld(), m_sLevelSelectionWorld, false, false);

		return;
	}

	uint32_t uiAllPlayersJoined = 0;
	uint32_t i = 0;

	m_bPlayer1Active = false;
	m_bPlayer2Active = false;

	for (auto& pStartToJoinComp : m_pStartToJoinComponents)
	{
		i++;

		if (!pStartToJoinComp)
			continue;

		if (pStartToJoinComp->PlayerJoined())
		{
			if (i == 1)
				m_bPlayer1Active = true;
			else
				m_bPlayer2Active = true;

			uiAllPlayersJoined++;
		}
	}

	if (m_uiPrevPlayerCount != uiAllPlayersJoined) {
		m_uiPrevPlayerCount = uiAllPlayersJoined;

		for (Entity* pEntity : m_pDeactivateEntitiesOnPlayersJoined)
		{
			if (pEntity)
				pEntity->SetEnabled(!(uiAllPlayersJoined > 0));
		}
	}
}
