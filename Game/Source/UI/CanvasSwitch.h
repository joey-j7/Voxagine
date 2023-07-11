#pragma once

#include "Core\ECS\Components\BehaviorScript.h"

class UIButton;
class Canvas;

class CanvasSwitch :
	public BehaviorScript
{
public:
	CanvasSwitch(Entity*);
	virtual ~CanvasSwitch();

	void Start() override;

private:

	UIButton* m_pButton = nullptr;

	Canvas* m_pFromCanvas = nullptr;
	Canvas* m_pToCanvas = nullptr;

	std::vector<uint64_t> m_InputBindings;

	bool m_bEnableBackButton = true;

	RTTR_ENABLE(Component)
	RTTR_REGISTRATION_FRIEND
};

