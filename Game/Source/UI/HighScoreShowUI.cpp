#include "pch.h"
#include "HighScoreShowUI.h"

#include <Core/ECS/Components/TextRenderer.h>

#include <External/rttr/registration>
#include <External/rttr/policy.h>

#include "Core/MetaData/PropertyTypeMetaData.h"
#include "HighScoreUI.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<HighScoreShowUI>("HighScoreShowUI")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Increment DeathCount", &HighScoreShowUI::m_bIncrementDeath)(RTTR_PUBLIC)
		.property("Reset on Start", &HighScoreShowUI::m_bResetOnStart)(RTTR_PUBLIC)
	;
}

void HighScoreShowUI::Awake()
{
	Entity::Awake();

	// Add Text Renderer component to the entity
	m_pTextRenderer = GetComponent<TextRenderer>();

	if (!m_pTextRenderer)
		m_pTextRenderer = AddComponent<TextRenderer>();

	if (m_bIncrementDeath)
		HighScoreUI::m_uiDeathCount++;

	if (HighScoreUI::m_fTimer > 0.f)
	{
		m_uiKillCount = HighScoreUI::m_uiKillCount;
		m_fTimer = HighScoreUI::m_fTimer;
		m_uiDeathCount = HighScoreUI::m_uiDeathCount;

		std::string hours = std::to_string(static_cast<uint32_t>(m_fTimer / (60.f * 60.f)));
		std::string minutes = std::to_string(static_cast<uint32_t>(std::fmod(m_fTimer / 60.f, 60.f)));
		std::string seconds = std::to_string(static_cast<uint32_t>(std::fmod(m_fTimer, 60.f)));

		if (hours.length() == 1)
			hours = "0" + hours;

		if (minutes.length() == 1)
			minutes = "0" + minutes;

		if (seconds.length() == 1)
			seconds = "0" + seconds;

		m_pTextRenderer->SetText(
			"Kill Count : " + std::to_string(m_uiKillCount) + " | Time: " +

			hours + ":" +
			minutes + ":" +
			seconds + " | Deaths: " + std::to_string(m_uiDeathCount)
		);
	}
	else
	{
		m_pTextRenderer->SetText(
			"Kill Count : 0 | Time: 00:00:00 | Deaths: 0"
		);
	}
}

void HighScoreShowUI::Start()
{
	Entity::Start();

	// Add Text Renderer component to the entity
	m_pTextRenderer = GetComponent<TextRenderer>();

	if (!m_pTextRenderer)
		m_pTextRenderer = AddComponent<TextRenderer>();

	if (m_bResetOnStart)
		HighScoreUI::Reset();
}