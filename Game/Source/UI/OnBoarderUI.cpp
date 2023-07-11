#include "OnBoarderUI.h"

#include "Core/ECS/Components/SpriteRenderer.h"

#include <Core/ECS/Components/BoxCollider.h>

#include <External\glm\gtx\compatibility.hpp>

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<OnBoarderUI>("OnBoarder UI")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Sprites", &OnBoarderUI::m_pSpriteRenderers) (RTTR_PUBLIC)

		.property("Start Color", &OnBoarderUI::m_StartColor) (RTTR_PUBLIC)
		.property("End Color", &OnBoarderUI::m_EndColor) (RTTR_PUBLIC)

		.property("Start Time", &OnBoarderUI::m_fStartTime) (RTTR_PUBLIC)
		.property("Start On Trigger", &OnBoarderUI::m_bStartOnTrigger) (RTTR_PUBLIC)

		.property("Showing Duration", &OnBoarderUI::m_fShowDuration) (RTTR_PUBLIC)
		.property("Transition Duration", &OnBoarderUI::m_fHideTransitionTime) (RTTR_PUBLIC)
	
		.property("Remove Entity When Completed", &OnBoarderUI::m_bRemoveEntityOnComplete) (RTTR_PUBLIC)
	;
}

OnBoarderUI::OnBoarderUI(Entity* pEntity)
	: BehaviorScript(pEntity)
{
}

OnBoarderUI::~OnBoarderUI()
{
}

void OnBoarderUI::Start()
{
	BehaviorScript::Start();

	for (auto& pSpriteRenderer : m_pSpriteRenderers)
		if (pSpriteRenderer)
			pSpriteRenderer->SetColor(m_StartColor);

	if (m_bStartOnTrigger)
	{
		// Collider
		m_pTriggerCollider = GetOwner()->GetComponent<BoxCollider>();
		if (m_pTriggerCollider == nullptr)
			m_pTriggerCollider = GetOwner()->AddComponent<BoxCollider>();

		m_pTriggerCollider->SetTrigger(true);
		m_pTriggerCollider->SetIgnoreVoxels(true);
	}
	else 
	{
		StartShowEffect(1.f, m_fStartTime);
	}
}

void OnBoarderUI::Tick(float fDeltaTime)
{
	BehaviorScript::Tick(fDeltaTime);

	if (m_fCurrentFadeDirection != 0.f)
	{
		m_fElapsedTime += fDeltaTime;

		float elapsedPercent = glm::clamp(m_fElapsedTime / m_fHideTransitionTime, 0.f, 1.f);

		for (auto& pSpriteRenderer : m_pSpriteRenderers)
		{
			if (!pSpriteRenderer)
				continue;

			if (m_fCurrentFadeDirection > 0)
			{
				pSpriteRenderer->SetColor(glm::lerp(m_StartColor.GetVector4(), m_EndColor.GetVector4(), glm::smoothstep(0.f, 1.f, elapsedPercent)));
			}
			else if (m_fCurrentFadeDirection < 0)
			{
				pSpriteRenderer->SetColor(glm::lerp(m_EndColor.GetVector4(), m_StartColor.GetVector4(), glm::smoothstep(0.f, 1.f, elapsedPercent)));
			}
		}

		if (elapsedPercent >= 1.f)
		{
			if (m_fCurrentFadeDirection > 0.f)
			{
				m_fCurrentFadeDirection = 0.f;
				StartShowEffect(-1.f, m_fShowDuration);
			}
			else 
			{
				if (m_bRemoveEntityOnComplete)
					GetOwner()->Destroy();
			}
		}
	}
}

void OnBoarderUI::OnCollisionEnter(Collider * pCollider, const Manifold & manifold)
{
	BehaviorScript::OnCollisionEnter(pCollider, manifold);

	auto& tags = pCollider->GetOwner()->GetTags();
	if (std::find(tags.begin(), tags.end(), "Player") != tags.end())
	{
		if (m_bStartOnTrigger)
			StartShowEffect(1.f, m_fStartTime);
	}
}

void OnBoarderUI::StartShowEffect(float fDirection, float fDelay)
{
	Invoke([this, fDirection]() {
		m_fElapsedTime = 0.f;
		m_fCurrentFadeDirection = fDirection;
	}, fDelay);
}
