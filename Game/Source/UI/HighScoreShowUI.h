#pragma once
#include "Core/ECS/Entity.h"

class TextRenderer;

class HighScoreShowUI : public Entity {
public:
	//An entity requires a world passed to its constructor
	HighScoreShowUI(World* world) : Entity(world) {};

	void Awake() override;
	void Start() override;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;

protected:
	TextRenderer* m_pTextRenderer = nullptr;

private:
	bool m_bIncrementDeath = false;
	bool m_bResetOnStart = false;

	float m_fTimer = 0.f;
	uint32_t m_uiKillCount = 0;
	uint32_t m_uiDeathCount = 0;
};

