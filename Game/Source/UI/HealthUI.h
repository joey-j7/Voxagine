#pragma once
#include "Core/ECS/Entity.h"

class SpriteRenderer;

class HealthUI : public Entity {
public:

	//An entity requires a world passed to its constructor
	HealthUI(World* world);

	void Awake() override;

	void AddComponents();

	void SetHealthCullingEnd(float currentHealth);

	std::string healthSprite = "Content/UI_Art/HUD_Sprites/Pixel_Heart.png";

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;

protected:
	
	SpriteRenderer* m_pSpriteRenderer = nullptr;

private:

	

};

