#pragma once

#include "Core/ECS/Components/BehaviorScript.h"

class StartToJoinPlayerComponent;
class UIButton;

class MainMenuManagerComponent :
	public BehaviorScript
{
public:
	MainMenuManagerComponent(Entity*);
	virtual ~MainMenuManagerComponent();

	void Start() override;
	void Tick(float) override;

	static uint32_t m_uiPlayerCount;
	static bool m_bPlayer1Active;
	static bool m_bPlayer2Active;

private:
	std::vector<StartToJoinPlayerComponent*> m_pStartToJoinComponents;
	UIButton* m_pLevelSelectButton = nullptr;
	std::vector<Entity*> m_pDeactivateEntitiesOnPlayersJoined;

	uint32_t m_uiPrevPlayerCount = 0;
	bool m_bGotoLevelSelection = false;

	std::string m_sLevelSelectionWorld;

	RTTR_ENABLE(BehaviorScript)
	RTTR_REGISTRATION_FRIEND

};

