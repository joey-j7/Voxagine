#pragma once

#include "AI/States/FSMState.h"

#include "General/Managers/WeaponManager.h"

class GameManager;
class BaseLoudOutBehavior;
class LoadOut;

class UIButton;
class SpriteRenderer;

class GM_LoadOutState : public FSMState<GameManager>
{
public:
	void Awake(GameManager*) override;
	void Start(GameManager*) override;
	void Tick(GameManager* pOwner, float fDeltaTime) override;
	void Exit(GameManager*) override;

protected:
	void SetCurrentResourceAmount();
	void SetIcons(UIButton* pButton);

	enum class ELoadOutCategory
	{
		EMovement,
		EPassive,
		EActivated,
		ERelics
	};

	void GrabLoadOuts();

	WeaponManager* m_pWeaponManager = nullptr;
	LoadOut* m_pLoadComponent = nullptr;

	float fCurrentPowerResources = 0;
	float fMaxAmountResources = 10;

	// UI parts
	ELoadOutCategory m_ECurrentCategory = ELoadOutCategory::EMovement;

	WeaponManager::Type* m_pSelected = nullptr;
	UIButton* m_pMoveMovementBtn = nullptr;
	UIButton* m_pPassiveBtn = nullptr;
	SpriteRenderer* m_pPowerbarSprite = nullptr;

	std::vector <WeaponManager::Type*> m_vCurrentSelection = {};
	std::multimap<ELoadOutCategory, WeaponManager::Type*> m_mBaseLoadOuts = {};

	bool m_bAdjusted = false;
};
