#include "pch.h"
#include "HighScoreUI.h"

#include <Core/ECS/Components/TextRenderer.h>

#include <External/rttr/registration>
#include <External/rttr/policy.h>

#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<HighScoreUI>("HighScoreUI")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Reset on Start", &HighScoreUI::m_bResetOnStart)(RTTR_PUBLIC)
	;
}

float HighScoreUI::m_fTimer = 0.f;
uint32_t HighScoreUI::m_uiKillCount = 0;
uint32_t HighScoreUI::m_uiDeathCount = 0;

void HighScoreUI::Awake()
{
	Entity::Awake();

	// Add Text Renderer component to the entity
	m_pTextRenderer = GetComponent<TextRenderer>();

	if (!m_pTextRenderer)
		m_pTextRenderer = AddComponent<TextRenderer>();

	m_bIsLeading = false;

	if (m_bResetOnStart)
	{
		m_uiKillCount = 0;
		m_fTimer = 0;
	}
}

void HighScoreUI::Start()
{
	Entity::Start();

	// Add Text Renderer component to the entity
	m_pTextRenderer = GetComponent<TextRenderer>();

	if (!m_pTextRenderer)
		m_pTextRenderer = AddComponent<TextRenderer>();

	bool bLeading = true;
	for (HighScoreUI* pEntity : GetWorld()->FindEntitiesOfType<HighScoreUI>())
	{
		if (pEntity->m_bIsLeading)
		{
			bLeading = false;
			break;
		}
	}

	SetPersistent(true);

	m_bIsLeading = bLeading;

	if (m_bResetOnStart)
	{
		m_uiKillCount = 0;
		m_fTimer = 0;
	}
}

void HighScoreUI::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);

	std::string hours = std::to_string(static_cast<uint32_t>(m_fTimer / (60.f * 60.f)));
	std::string minutes = std::to_string(static_cast<uint32_t>(std::fmod(m_fTimer / 60.f, 60.f)));
	std::string seconds = std::to_string(static_cast<uint32_t>(std::fmod(m_fTimer, 60.f)));

	if (hours.length() == 1)
		hours = "0" + hours;

	if (minutes.length() == 1)
		minutes = "0" + minutes;

	if (seconds.length() == 1)
		seconds = "0" + seconds;

	if (m_bIsLeading)
	{
		m_fTimer += fDeltaTime;
	}

	m_pTextRenderer->SetText(
		"Kill Count : " + std::to_string(m_uiKillCount) + " | Time: " +

		hours + ":" +
		minutes + ":" +
		seconds
	);
}

void HighScoreUI::IncrementKillCount()
{
	m_uiKillCount++;
}

void HighScoreUI::Reset()
{
	m_fTimer = 0.f;
	m_uiKillCount = 0;
	m_uiDeathCount = 0;
}
