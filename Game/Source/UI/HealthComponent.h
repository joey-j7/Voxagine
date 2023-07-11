#pragma once

#include <array>

#include "Core/ECS/Components/BehaviorScript.h"

class SpriteRenderer;
class GameManager;
class HealthComponent : public BehaviorScript
{
	RTTR_ENABLE(BehaviorScript)
	RTTR_REGISTRATION_FRIEND
public:
	HealthComponent(Entity* pOwner);

	void Start() override;
	void Tick(float fDeltaTime) override;

	void Reset();

	GameManager* m_pGameManager = nullptr;
	std::array<SpriteRenderer*, 5> m_HealthBars = {};
private:
	void SetFullHearth(SpriteRenderer* pRenderer);
	void SetHalfHearth(SpriteRenderer* pRenderer);
	void SetEmptyHearth(SpriteRenderer* pRenderer);
	std::string m_sFilePathFull = "Content/UI_Art/HUD_Sprites/Health_Full.png";
	std::string m_sFilePathHalf = "Content/UI_Art/HUD_Sprites/Half Hearth.png";
	std::string m_sFilePathEmpty = "Content/UI_Art/HUD_Sprites/Health_Empty.png";

	double m_CurrentHealth = 5.0;
};
