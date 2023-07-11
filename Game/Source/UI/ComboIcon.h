#pragma once
#include "Core/ECS/Entity.h"

class SpriteRenderer;

class ComboIcon : public Entity {
public:

	//An entity requires a world passed to its constructor
	ComboIcon(World* world);

	void Awake() override;

	void AddComponents();

	void SetComboIconImage(int currentComboThreshold);

	SpriteRenderer* m_pSpriteRenderer;

	std::string combo0Image = "";
	std::string combo1Image = "";
	std::string combo2Image = "";
	std::string combo3Image = "";

	RTTR_ENABLE(Entity)
		RTTR_REGISTRATION_FRIEND;

protected:

private:

};