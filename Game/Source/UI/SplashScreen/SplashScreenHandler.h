#pragma once

#include "Core\ECS\Components\BehaviorScript.h"

class SpriteRenderer;

class SplashScreenHandler :
	public BehaviorScript
{
public:
	SplashScreenHandler(Entity*);
	virtual ~SplashScreenHandler();

	void Start() override;

	void Tick(float) override;

protected:

	std::vector<uint64_t> m_InputBindings;

private:

	std::vector<SpriteRenderer*> m_pSpriteRenderers;

	float m_fWorldChangeTime = 4.f;
	float m_fShowDuration = 2.f;

	float m_fStartTime = .3f;

	float m_fFadeDuration = .9f;
	float m_fFadeDirection = 0.f;

	std::string m_sMainMenuWorld = "";

	float m_fCurrentProgress = 0.f;

	RTTR_ENABLE(BehaviorScript)
	RTTR_REGISTRATION_FRIEND
};

