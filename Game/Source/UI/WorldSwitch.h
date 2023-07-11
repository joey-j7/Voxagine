#pragma once

#include "Core\ECS\Components\BehaviorScript.h"

class UIButton;
class World;

class WorldSwitch :
	public BehaviorScript
{
public:
	WorldSwitch(Entity*);
	virtual ~WorldSwitch();

	void Start() override;
	void Tick(float fDeltaTime) override;

	static void SwitchWorld(World* m_pWorld, std::string m_sWorldFilepath = "", bool m_bLoadAsync = false, bool m_bUseLoadingScreen = true);

private:

	UIButton* m_pButton = nullptr;
	std::string m_sWorldFilename = "";

	bool m_bLoadPreviousWorld = false;

	bool m_bAsync = false;
	bool m_bUseLoadingScreen = true;

	bool m_bFadeBGM = false;
	float m_fBGMVolume = 1.f;

	bool m_bClicked = false;

	uint32_t m_uiWorldToPop = 0;

	RTTR_ENABLE(Component)
	RTTR_REGISTRATION_FRIEND
};

