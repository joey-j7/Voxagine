#pragma once

#include "AI/States/FSMState.h"
#include "Core/Math.h"

class GameManager;

class MenuState : public FSMState<GameManager> 
{
public:
	MenuState() = default;
	virtual ~MenuState() = default;

	void Start(GameManager* pOwner) override;
	void Tick(GameManager* pOwner, float) override;
	void Exit(GameManager* pOwner) override;
};

