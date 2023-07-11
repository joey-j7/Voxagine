#pragma once

#include "Core/ECS/Components/BehaviorScript.h"

class Player;
class UISlider;

class DashCooldownComponent : public BehaviorScript
{
public:
	DashCooldownComponent(Entity*);

	void Tick(float) override;

private:

	Player* m_pPlayer = nullptr;

	UISlider* m_pDashCooldownSlider;

	RTTR_ENABLE(BehaviorScript)
	RTTR_REGISTRATION_FRIEND
};

