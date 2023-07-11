#include "HealthComponent.h"
#include "Core/ECS/Components/SpriteRenderer.h"

#include "General/Managers/GameManager.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<HealthComponent>("HealthComponent")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("GameManager", &HealthComponent::m_pGameManager) (RTTR_PUBLIC)
		.property("Health Bars", &HealthComponent::m_HealthBars) (RTTR_PUBLIC)
		.property("Full Hearth Ref", &HealthComponent::m_sFilePathFull) (RTTR_PUBLIC, RTTR_RESOURCE("png"))
		.property("Half Hearth Ref", &HealthComponent::m_sFilePathHalf) (RTTR_PUBLIC, RTTR_RESOURCE("png"))
		.property("Empty Health Ref", &HealthComponent::m_sFilePathEmpty) (RTTR_PUBLIC, RTTR_RESOURCE("png"));
}

HealthComponent::HealthComponent(Entity* pOwner) : BehaviorScript(pOwner) {}

void HealthComponent::Start()
{
	Reset();
}

void HealthComponent::Tick(float fDeltaTime)
{
	if(m_pGameManager)
	{
		const double health = static_cast<double>(m_pGameManager->GetHealth());
		if (m_CurrentHealth != health)
		{
			if(health == 4.5)
			{
				SetHalfHearth(m_HealthBars[4]);
			} else if(health == 4)
			{
				SetEmptyHearth(m_HealthBars[4]);
			} else if(health == 3.5)
			{
				SetHalfHearth(m_HealthBars[3]);
			} else if(health == 3)
			{
				SetEmptyHearth(m_HealthBars[3]);
			}
			else if (health == 2.5)
			{
				SetHalfHearth(m_HealthBars[2]);
			}
			else if (health == 2)
			{
				SetEmptyHearth(m_HealthBars[2]);
			}
			else if (health == 1.5)
			{
				SetHalfHearth(m_HealthBars[1]);
			}
			else if (health == 1)
			{
				SetEmptyHearth(m_HealthBars[1]);
			}
			else if (health == .5)
			{
				SetHalfHearth(m_HealthBars[0]);
			}
			else if (health == 0)
			{
				SetEmptyHearth(m_HealthBars[0]);
			}
			
			m_CurrentHealth = health;
		}
	}
}

void HealthComponent::Reset()
{
	for (uint32_t i = 0; i < m_HealthBars.size(); ++i)
	{
		auto healthBar = m_HealthBars[i];
		if (healthBar)
		{
			healthBar->SetFilePath(m_sFilePathFull);
		}
	}
}

void HealthComponent::SetFullHearth(SpriteRenderer* pRenderer)
{
	if (pRenderer)
	{
		pRenderer->SetFilePath(m_sFilePathFull);
	}
}

void HealthComponent::SetHalfHearth(SpriteRenderer* pRenderer)
{
	if (pRenderer)
	{
		pRenderer->SetFilePath(m_sFilePathHalf);
	}
}

void HealthComponent::SetEmptyHearth(SpriteRenderer* pRenderer)
{
	if (pRenderer)
	{
		pRenderer->SetFilePath(m_sFilePathEmpty);
	}
}
