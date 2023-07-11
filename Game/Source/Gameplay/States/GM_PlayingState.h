#pragma once

#include "AI/States/FSMState.h"
#include "General/Managers/GameManager.h"

class GM_PlayingState : public FSMState<GameManager>
{
public:
	void Start(GameManager*) override;
	void Tick(GameManager* pOwner, float fDeltaTime) override;
	void Exit(GameManager*) override;
};