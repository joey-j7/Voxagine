#include "GM_LoadoutState.h"

#include "Core/Application.h"
#include "Core/LoggingSystem/LoggingSystem.h"

#include "Core/PlayerPrefs/PlayerPrefs.h"

#include "UI/Loadout.h"

#include "General/Managers/GameManager.h"
#include "Core/ECS/Components/UI/UIButton.h"
#include "Core/ECS/Components/SpriteRenderer.h"

void GM_LoadOutState::Awake(GameManager* pGameManager)
{
	auto entities = pGameManager->GetWorld()->FindEntitiesOfType<WeaponManager>();

	if (!entities.empty())
	{
		m_pWeaponManager = entities[0];

		// Grab the things the player had.
		for (auto& type : m_pWeaponManager->m_Types)
		{
			if (!type.m_bUnlocked)
				continue;

			// check if we have the movement which are not straight
			if (type.m_MovementType >= WeaponManager::Type::MT_SINE)
				m_mBaseLoadOuts.insert({ ELoadOutCategory::EMovement, &type });

			// grab all the activated types
			if (type.m_ActivatedType > WeaponManager::Type::AT_NONE && type.m_MovementType == WeaponManager::Type::MovementType::MT_STRAIGHT)
				m_mBaseLoadOuts.insert({ ELoadOutCategory::EActivated, &type });

			// TODO Add passive types
			// if (!type.m_ActivatedType > WeaponManager::Type::AT_NONE && type.m_MovementType == WeaponManager::Type::MovementType::MT_STRAIGHT)
				// m_mBaseLoadOuts.insert({ ELoadOutCategory::EActivated, &type });
		}
	}

	if (auto pEntity = pGameManager->GetWorld()->FindEntity("Movement Button"))
	{
		m_pMoveMovementBtn = pEntity->GetComponent<UIButton>();

		// 
		if (m_pMoveMovementBtn)
		{
			m_pMoveMovementBtn->m_FocusEvent += Event<UIButton*>::Subscriber([=](UIButton*)
			{
				m_ECurrentCategory = ELoadOutCategory::EMovement;
				m_vCurrentSelection.clear();
				const auto& itr = m_mBaseLoadOuts.equal_range(m_ECurrentCategory);
				for (auto it = itr.first; it != itr.second; ++it)
				{
					m_vCurrentSelection.push_back(it->second);
				}
				m_bAdjusted = false;

			}, this);
			m_pMoveMovementBtn->SetIsFocusable(true);
		}
	}

	if (auto pEntity = pGameManager->GetWorld()->FindEntity("Passive Button"))
	{
		m_pPassiveBtn = pEntity->GetComponent<UIButton>();
		if (m_pPassiveBtn)
		{
			m_pPassiveBtn->m_FocusEvent += Event<UIButton*>::Subscriber([=](UIButton*)
			{
				m_ECurrentCategory = ELoadOutCategory::EPassive;
				m_vCurrentSelection.clear();
				const auto& itr = m_mBaseLoadOuts.equal_range(m_ECurrentCategory);
				for (auto it = itr.first; it != itr.second; ++it)
				{
					m_vCurrentSelection.push_back(it->second);
				}
				m_bAdjusted = false;
			}, this);

		}
	}
}

void GM_LoadOutState::Start(GameManager* pGameManager)
{
	// Find the powerbar
	if(auto pPowerBarEntity = pGameManager->GetWorld()->FindEntity("Powerbar"))
	{
		m_pPowerbarSprite = pPowerBarEntity->GetComponent<SpriteRenderer>();
	}

	if (!pGameManager->vCurrentButtons.empty())
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			auto pButton = pGameManager->vCurrentButtons[i];
			if (pButton == nullptr)
				continue;

			// Use the four first button to set the icons to the current category
			pButton->m_ClickedEvent += Event<UIButton*>::Subscriber([=](UIButton*)
			{
				if (!m_pSelected)
					return;

				switch (m_ECurrentCategory)
				{
				case ELoadOutCategory::EMovement:
				{
					m_pSelected->m_bSelected = !m_pSelected->m_bSelected;
					if (m_pSelected->m_bSelected)
					{
						pGameManager->GetWorld()->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_MESSAGE, "Increasing");
						if (fCurrentPowerResources < fMaxAmountResources)
						{
							fCurrentPowerResources += m_pSelected->m_fResourceAmount;
							SetCurrentResourceAmount();
						}
						m_pWeaponManager->SetTypeID(m_vCurrentSelection[i]->m_MovementType);
					}
					else
					{
						pGameManager->GetWorld()->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_MESSAGE, "Decreasing");
						fCurrentPowerResources -= m_pSelected->m_fResourceAmount;
						SetCurrentResourceAmount();

						// set it back to the normal one
						m_pWeaponManager->SetTypeID(WeaponManager::Type::MT_STRAIGHT);
					}
				}
				break;
				case ELoadOutCategory::EActivated:
				{
					// we can only have one
					m_pSelected->m_bSelected = !m_pSelected->m_bSelected;
					if (m_pSelected->m_bSelected)
					{
						if (fCurrentPowerResources < fMaxAmountResources)
						{
							fCurrentPowerResources += m_pSelected->m_fResourceAmount;
							SetCurrentResourceAmount();
						}
						m_pWeaponManager->SetTypeID(m_vCurrentSelection[i]->m_MovementType);
					}
					else
					{
						fCurrentPowerResources -= m_pSelected->m_fResourceAmount;
						SetCurrentResourceAmount();

						// set it back to the normal one
						m_pWeaponManager->SetTypeID(WeaponManager::Type::AT_NONE);
					}
				}
				break;
				case ELoadOutCategory::ERelics:
					break;
				case ELoadOutCategory::EPassive:
					break;
				}

			}, this);
		}
	}

	m_pLoadComponent = pGameManager->GetComponent<LoadOut>();
}

void GM_LoadOutState::Tick(GameManager* pGameManager, float fDeltaTime)
{
	// Check the selection
	if(!pGameManager->vCurrentButtons.empty())
	{
		// Loop through the selections
		for(size_t i = 0; i < pGameManager->vCurrentButtons.size(); ++i)
		{
			// TODO we need to know on which page we are.

			auto pButton = pGameManager->vCurrentButtons[i];

			// if we have this amount of selection based on the bu
			if (i < m_vCurrentSelection.size())
			{
				m_pSelected = m_vCurrentSelection[i];
				pButton->GetOwner()->SetEnabled(true);

				SetIcons(pButton);
			}
			else
				pButton->GetOwner()->SetEnabled(false);
		}
	}
}

void GM_LoadOutState::Exit(GameManager*)
{
	if(m_pMoveMovementBtn)
		m_pMoveMovementBtn->m_FocusEvent -= this;
}

void GM_LoadOutState::SetCurrentResourceAmount()
{
	// keep it below 10
	fCurrentPowerResources = std::min(fCurrentPowerResources, fMaxAmountResources);
	m_pPowerbarSprite->SetFilePath("Content/UI/Loadout/Power " + std::to_string(fCurrentPowerResources) + ".png");
}

void GM_LoadOutState::SetIcons(UIButton* pButton)
{
	if (m_bAdjusted)
		return;
	
	if (auto pUIComponent = pButton->m_NormalObject->GetComponent<SpriteRenderer>())
	{
		pUIComponent->SetFilePath(m_pSelected->m_IconPath);
	}

	// if (auto pUIComponent = pButton->m_DisabledObject->GetComponent<SpriteRenderer>())
	// {
		// pUIComponent->SetFilePath(pSelection->m_IconPath);
	// }

	if (auto pUIComponent = pButton->m_FocusedObject->GetComponent<SpriteRenderer>())
	{
		pUIComponent->SetFilePath(m_pSelected->m_IconPath);
	}

	m_bAdjusted = true;
}


void GM_LoadOutState::GrabLoadOuts()
{
	// TODO grab the loadouts
}
