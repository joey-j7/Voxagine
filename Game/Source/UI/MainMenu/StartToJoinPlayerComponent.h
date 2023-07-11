#pragma once

#include "Core/ECS/Components/BehaviorScript.h"

class Player;
class VoxAnimator;
class AudioSource;

class StartToJoinPlayerComponent 
	: public BehaviorScript
{
public:
	StartToJoinPlayerComponent(Entity*);
	virtual ~StartToJoinPlayerComponent();

	void Awake() override;
	void Start() override;

	void Tick(float) override;

	bool PlayerJoined() const {
		return m_bPlayerJoined;
	}

	void ShowErrorAnim();

private:

	void TogglePlayerShowing();

	void SetJoinVisualActive(int);

	void SetPlayerAnimIndex(unsigned int);

private:

	bool m_bPlayerJoined = false;

	std::vector<Entity*> m_pStartToJoinVisuals;
	std::vector<Entity*> m_pJoinedVisuals;
	float m_fFlickerInterval = .4f;

	float m_fProgressIntervalTime = 0.f;
	int m_iCurrentStartToJoinVisual = 0;

	Player* m_pPlayer = nullptr;

	float m_fPlayerErrorInterval = .1f;
	float m_fPlayerErrorProgress = 0.f;
	int m_iPlayerErrorSwitchTimes = 4;
	int m_iPlayerErrorCurrentSwitchTimes = 0;
	VoxAnimator* m_pPlayerRenderer = nullptr;

	unsigned int m_iDefaultPlayerAnimIndex = 0;
	unsigned int m_iDisabledPlayerAnimIndex = 1;
	unsigned int m_iErrorPlayerAnimIndex = 2;
	
	bool m_bShowErrorVisual = false;
	bool m_bShowErrorAnim = false;

	AudioSource* m_pAudioSource = nullptr;

	RTTR_ENABLE(BehaviorScript)
	RTTR_REGISTRATION_FRIEND
};

