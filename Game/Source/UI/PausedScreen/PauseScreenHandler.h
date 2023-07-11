#pragma once

#include "Core\ECS\Components\BehaviorScript.h"

class Canvas;
class UIButton;

class PauseScreenHandler :
	public BehaviorScript
{
public:
	PauseScreenHandler(Entity*);
	virtual ~PauseScreenHandler();

	void Awake() override;
	void Start() override;

	void Tick(float fDeltaTime) override;

	void ResumeGame();

	Canvas* m_pPausedCanvas = nullptr;
	std::vector<UIButton*> m_pResumeButtons;

private:
	bool m_bResumed = false;

	std::vector<uint64_t> m_InputBindings;

	RTTR_ENABLE(BehaviorScript);
	RTTR_REGISTRATION_FRIEND
};

