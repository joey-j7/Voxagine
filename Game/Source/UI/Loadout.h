#pragma once

#include <vector>

#include "Core/ECS/Components/UI/UIComponent.h"

class BaseLoudOutBehavior;
class BaseMovement;
class BasRelic;
class BasePassive;
class BaseActivated;
class LoadOut : public UIComponent
{
public:
	LoadOut(Entity* pOwner) : UIComponent(pOwner) {}

	void AssignLoadOut(BaseLoudOutBehavior* pLoadOut);

	float fCurrentPowerLevel = 0.0f;
	float fCurrentAmountResources = 30.f;
protected:
	BaseMovement* m_pMovement = nullptr;
	BasRelic* m_pRelic = nullptr;
	std::vector<BasePassive*> m_vPassivePerks = {};
	std::vector<BaseActivated*> m_vActivatedPerks = {};
};
