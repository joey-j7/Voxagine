#include "pch.h"
#include "AudioPlaylist.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "AudioSource.h"

#include <Core/Application.h>
#include <Core/Resources/ResourceManager.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<AudioPlaylist>("AudioPlaylist")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Current Audio", &AudioPlaylist::GetCurrentIndex, &AudioPlaylist::SetCurrentIndex) (RTTR_PUBLIC)
	;
}


AudioPlaylist::AudioPlaylist(Entity* pOwner) :
	Component(pOwner)
{
	Requires<AudioSource>();
}

AudioPlaylist::~AudioPlaylist()
{
	ClearAudioFiles();
}

void AudioPlaylist::OnEnabled()
{
	m_pAudioSource = GetOwner()->GetComponent<AudioSource>();
}

void AudioPlaylist::OnDisabled()
{
	m_pAudioSource = nullptr;
}

void AudioPlaylist::Awake()
{
	Component::Awake();
	m_pAudioSource = GetOwner()->GetComponent<AudioSource>();
}

void AudioPlaylist::Play(uint32_t uiIndex, bool bForce)
{
	uiIndex = std::min(uiIndex, (uint32_t)m_SoundReferences.size());

	if (bForce || uiIndex != m_uiCurrentIndex)
	{
		SetCurrentIndex(uiIndex);
		SoundReference* pReference = GetActiveSoundReference();

		if (m_pAudioSource && pReference)
		{
			m_pAudioSource->Play();
		}
	}
}

void AudioPlaylist::PlayNext()
{
	uint32_t id = (m_uiCurrentIndex + 1) % m_SoundReferences.size();
	Play(id);
}

void AudioPlaylist::AddAudioFile(SoundReference* audioFile, unsigned int audioFileIndex)
{
	if (audioFileIndex >= 0 && audioFileIndex <= m_SoundReferences.size())
	{
		if (audioFileIndex >= m_SoundReferences.size())
		{
			while (audioFileIndex >= m_SoundReferences.size())
				m_SoundReferences.push_back(nullptr);
		}
		else
		{
			if (m_SoundReferences[audioFileIndex])
				RemoveAudioFile(audioFileIndex);
		}

		if (audioFile)
		{
			if (audioFileIndex == m_SoundReferences.size())
			{
				m_SoundReferences.push_back(nullptr);
			}
		}

		m_SoundReferences[audioFileIndex] = audioFile;

		SetCurrentIndex(GetCurrentIndex());
	}
}

void AudioPlaylist::RemoveAudioFile(unsigned int audioFileIndex)
{
	if (audioFileIndex >= 0 && audioFileIndex < m_SoundReferences.size())
	{
		if (audioFileIndex == m_SoundReferences.size() - 1)
		{
			m_SoundReferences.pop_back();
		}
		else
		{
			m_SoundReferences[audioFileIndex] = nullptr;
		}
	}
}

void AudioPlaylist::ClearAudioFiles()
{
	if (m_SoundReferences.empty())
		return;

	for (unsigned int audioFileIt = 0; audioFileIt <= m_SoundReferences.size(); ++audioFileIt)
		RemoveAudioFile(audioFileIt);
}

SoundReference* AudioPlaylist::GetActiveSoundReference() const
{
	if (m_SoundReferences.size() != 0 && m_uiCurrentIndex < m_SoundReferences.size())
	{
		return m_SoundReferences[m_uiCurrentIndex];
	}

	return nullptr;
}

void AudioPlaylist::SetCurrentIndex(uint32_t uiCurrentIndex)
{
	m_uiCurrentIndex = uiCurrentIndex;
	SoundReference* pReference = GetActiveSoundReference();

	if (pReference && m_pAudioSource)
	{
		m_pAudioSource->SetSoundReference(pReference, true);
	}
}
