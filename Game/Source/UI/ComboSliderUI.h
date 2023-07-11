#pragma once
#include "Core/ECS/Entity.h"

class SpriteRenderer;
class TextRenderer;
class UISlider;

class ComboSliderUI : public Entity {
public:

	//An entity requires a world passed to its constructor
	ComboSliderUI(World* world);

	void Awake() override;

	void AddComponents();

	UISlider* m_pComboSlider;

	void SetComboSlider(int currentComboStreak, int comboGoal);

	RTTR_ENABLE(Entity)
		RTTR_REGISTRATION_FRIEND;

protected:

	TextRenderer* m_pTextRenderer = nullptr;

private:



};