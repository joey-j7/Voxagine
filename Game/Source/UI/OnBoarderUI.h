#pragma once

#include "Core\ECS\Components\BehaviorScript.h"
#include "Core/VColors.h"

class Entity;
class SpriteRenderer;

class OnBoarderUI :
	public BehaviorScript
{
public:
	OnBoarderUI(Entity*);
	virtual ~OnBoarderUI();

	void Start() override;
	void Tick(float) override;

	void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) override;

private:

	void StartShowEffect(float, float = 0.f);

private:

	std::vector<SpriteRenderer*> m_pSpriteRenderers;

	VColor m_StartColor = VColors::White;
	VColor m_EndColor = VColors::White;

	float m_fStartTime = 0.f;
	bool m_bStartOnTrigger = false;

	float m_fShowDuration = 5.f;
	float m_fHideTransitionTime = .5f;

	bool m_bRemoveEntityOnComplete = false;

	float m_fElapsedTime = 0.f;
	float m_fCurrentFadeDirection = 0.f;

	Collider* m_pTriggerCollider = nullptr;

	RTTR_ENABLE(BehaviorScript);
	RTTR_REGISTRATION_FRIEND
};

