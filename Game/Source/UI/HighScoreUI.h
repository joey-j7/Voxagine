#pragma once
#include "Core/ECS/Entity.h"

class TextRenderer;

class HighScoreUI : public Entity {
public:
	friend class HighScoreShowUI;

	//An entity requires a world passed to its constructor
	HighScoreUI(World* world) : Entity(world) {};

	void Awake() override;
	void Start() override;

	void Tick(float fDeltaTime) override;
	static void IncrementKillCount();

	static void Reset();

	bool m_bIsLeading = false;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND;

protected:
	TextRenderer* m_pTextRenderer = nullptr;

private:
	static float m_fTimer;
	static uint32_t m_uiKillCount;
	static uint32_t m_uiDeathCount;

	bool m_bResetOnStart = true;
};

