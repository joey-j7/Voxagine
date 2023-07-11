#include "pch.h"
#include "OpenLevelTrigger.h"

#include "UI/WorldSwitch.h"

#include "Core/PlayerPrefs/PlayerPrefs.h"

#include <Core/ECS/Components/BoxCollider.h>
#include <External/rttr/registration.h>
#include <Core/MetaData/PropertyTypeMetaData.h>
#include "Core/ECS/World.h"

#include "Core/Application.h"
#include <Core/Platform/Platform.h>
#include <Core/Platform/Audio/AudioContext.h>
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include <Core/ECS/Components/AudioSource.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<OpenLevelTrigger>("Level Completed Trigger")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)

		.property("World Name", &OpenLevelTrigger::m_sNextWorldFilepath)(RTTR_PUBLIC, RTTR_RESOURCE("wld"))
		.property("Worlds To Unlock", &OpenLevelTrigger::m_sUnlockingWorlds)(RTTR_PUBLIC, RTTR_RESOURCE("wld"))

		.property("Fade Time", &OpenLevelTrigger::GetFadeTime, &OpenLevelTrigger::SetFadeTime)(RTTR_PUBLIC)
		.property("Stop Music", &OpenLevelTrigger::m_bStopMusic)(RTTR_PUBLIC)
	;
}

OpenLevelTrigger::OpenLevelTrigger(World * world) :
	Entity(world)
{
	m_pAudioContext = GetWorld()->GetApplication()->GetPlatform().GetAudioContext();

	m_pAudioSource = GetComponent<AudioSource>();
	if (!m_pAudioSource)
		m_pAudioSource = AddComponent<AudioSource>();

	m_pAudioSource->SetLooping(false);
	m_pAudioSource->SetFilePath("Content/SFX/PlayerStairs.ogg");
	m_pAudioSource->Set3DAudio(false);
}

void OpenLevelTrigger::Awake()
{
	Entity::Awake();
	SetName("Level Completed Trigger");
}

void OpenLevelTrigger::Start()
{
	Entity::Start();

	m_pAudioSource = GetComponent<AudioSource>();
	if (!m_pAudioSource)
		m_pAudioSource = AddComponent<AudioSource>();

	m_pAudioSource->SetLooping(false);
	m_pAudioSource->SetFilePath("Content/SFX/PlayerStairs.ogg");
	m_pAudioSource->Set3DAudio(false);

	// Collider
	BoxCollider* boxcollider = GetComponent<BoxCollider>();
	if (boxcollider == nullptr)
		boxcollider = AddComponent<BoxCollider>();

	boxcollider->SetTrigger(true);
	boxcollider->SetIgnoreVoxels(true);
}

void OpenLevelTrigger::OnCollisionEnter(Collider * pCollider, const Manifold & manifold)
{
	if (m_bIsTriggered)
		return;

	auto& tags = pCollider->GetOwner()->GetTags();
	if (std::find(tags.begin(), tags.end(), "Player") != tags.end())
	{
		m_fBGMVolume = m_pAudioContext->GetBGMVolume();

		GetWorld()->GetRenderSystem()->SetFadeTime(m_fFadeTime);
		GetWorld()->GetRenderSystem()->Fade();

		// Play audio
		m_pAudioSource->Play();

		UnlockWorlds(m_sUnlockingWorlds);
		m_bIsTriggered = true;
	}
}

void OpenLevelTrigger::Tick(float fDeltaTime)
{
	if (!m_bIsTriggered)
		return;

	if (m_fFadeTime > 0.f)
	{
		if (m_fFadeTimer < m_fFadeTime)
		{
			m_fFadeTimer = std::min(m_fFadeTime, m_fFadeTimer + fDeltaTime);
		}

		if (m_bStopMusic)
		{
			float fBGMVolume = m_pAudioContext->GetBGMVolume();

			if (fBGMVolume > 0.f)
			{
				m_pAudioContext->SetBGMVolume(
					(m_fFadeTime - m_fFadeTimer) / m_fFadeTime * m_fBGMVolume
				);

				return;
			}

			m_pAudioContext->StopBGM();
		}

		if (m_fFadeTimer < m_fFadeTime)
			return;
	}

	// Open the menu
	WorldSwitch::SwitchWorld(GetWorld(), m_sNextWorldFilepath, false, true);
}

void OpenLevelTrigger::UnlockWorlds(std::vector<std::string> m_sWorldPaths)
{
	for (auto& sWorldToUnlock : m_sWorldPaths)
	{
		if(!PlayerPrefs::GetBoolAccessor().Get("Unlocked" + sWorldToUnlock, false))
			PlayerPrefs::GetBoolAccessor().Set("NewUnlocked" + sWorldToUnlock, true);
	}

	PlayerPrefs::Save();
}

void OpenLevelTrigger::SetFadeTime(float fFadeTime)
{
	m_fFadeTime = std::max(0.f, fFadeTime);
}
