#pragma once
#include "Core/ECS/Entity.h"

class SpriteRenderer;
class TextRenderer;

class ComboUI : public Entity {
public:

	//An entity requires a world passed to its constructor
	ComboUI(World* world);

	void Awake() override;

	//Get the renderer
	TextRenderer* GetTextRenderer() const
	{
		return m_pTextRenderer;
	}

	void AddComponents();

	void SetComboUI(std::string currentCombo);

	RTTR_ENABLE(Entity)
		RTTR_REGISTRATION_FRIEND;

protected:

	TextRenderer* m_pTextRenderer = nullptr;

private:



};