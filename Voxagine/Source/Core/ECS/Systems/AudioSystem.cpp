#include "pch.h"
#include "AudioSystem.h"

#include "Core/Platform/Audio/AudioContext.h"
#include "../Components/AudioSource.h"

#include <Core/Platform/Platform.h>
#include <Core/Application.h>
#include "../World.h"
#include "External/optick/optick.h"

#include "Core/ECS/Entities/Camera.h"

AudioSystem::AudioSystem(World* pWorld)
	: ComponentSystem(pWorld)
{
	m_pWorld->Paused += Event<World*>::Subscriber(std::bind(&AudioSystem::OnWorldPaused, this, std::placeholders::_1), this);
	m_pWorld->Resumed += Event<World*>::Subscriber(std::bind(&AudioSystem::OnWorldResumed, this, std::placeholders::_1), this);

	m_pAudioContext = pWorld->GetApplication()->GetPlatform().GetAudioContext();
}

AudioSystem::~AudioSystem()
{
	
}

void AudioSystem::Start()
{
	if (!m_pWorld->GetApplication()->IsInEditor())
	{
		for (auto& it : m_AudioSources)
		{
			if (it.first->m_bAutoPlay)
			{
				if (it.first->IsBGM())
				{
					it.first->SetAsBGM(0.f);
					continue;
				}

				it.first->Play();
				continue;
			}
		}
	}
}

bool AudioSystem::CanProcessComponent(Component* pComponent)
{
	return dynamic_cast<AudioSource*>(pComponent);
}

void AudioSystem::Tick(float fDeltaTime)
{
	OPTICK_CATEGORY("Audio", Optick::Category::Audio);
	OPTICK_EVENT();
	/* BGM logic */
	if (m_fBGMTimer < m_fBGMMaxTimer)
	{
		m_fBGMTimer += fDeltaTime;

		if (m_pBGM) {
			m_pAudioContext->SetBGMVolume(std::max(0.0f, (m_fBGMMaxTimer - m_fBGMTimer) / m_fBGMMaxTimer));
		}

		if (m_bBGMCrossFade && m_pNextBGM)
		{
			// Unsupported
			assert(false);

			// m_pNextBGM->SetVolume(std::min(1.0f, m_fBGMTimer / m_fBGMMaxTimer));
		}

		if (m_fBGMTimer >= m_fBGMMaxTimer)
		{
			if (m_pBGM)
			{
				if (m_bBGMStopNext)
					m_pAudioContext->StopBGM();
				else
				{
					m_pAudioContext->PauseBGM();
					m_pBGM->SetPosition(m_pAudioContext->GetPlaybackPosition(m_pAudioContext->GetBGMChannel()));
				}
			}

			if (m_pNextBGM)
			{
				m_pBGM = m_pNextBGM;
				m_pNextBGM = nullptr;

				m_pAudioContext->SetBGMVolume(m_pBGM->GetVolume());
				m_pAudioContext->PlayBGM(m_pBGM->GetSoundReference(), m_pBGM->GetVolume(), m_pBGM->GetLoopStart(), m_pBGM->GetLoopEnd());
			}
		}
	}
}

void AudioSystem::PostTick(float fDeltaTime)
{
	OPTICK_CATEGORY("Audio", Optick::Category::Audio);
	OPTICK_EVENT();

	// Update 3D location
	Camera* pCamera = m_pWorld->GetMainCamera();
	Transform* pCamTransform = pCamera->GetTransform();

	if (m_v3LastCameraPosition == Vector3(-FLT_MAX))
	{
		m_v3LastCameraPosition = pCamTransform->GetPosition();
	}

	m_pAudioContext->Set3DSystemParameters(
		pCamTransform->GetPosition(),
		pCamTransform->GetPosition() - m_v3LastCameraPosition,
		pCamTransform->GetForward(),
		pCamTransform->GetUp()
	);

	m_v3LastCameraPosition = pCamTransform->GetPosition();

	/* Update BGM playback */
	if (m_pAudioContext->GetBGMChannel())
	{
		if (m_pAudioContext->GetBGMReference()->GetRefPath().find("_BGM") == std::string::npos)
		{
			m_pAudioContext->Set3DParameters(
				m_pAudioContext->GetBGMChannel(),
				pCamTransform->GetPosition(),
				Vector3(0.f)
			);
		}
	}

	/* Update playback position */
	for (auto& it : m_AudioSources)
	{
		AudioSource* pSource = it.first;

		if (pSource->IsEnabled()) {
			if (it.second == AS_PLAYING)
			{
				for (auto& it2 : pSource->m_pGroups)
				{
					for (uint32_t i = 0; i < it2.second.size(); ++i)
					{
						if (it2.second[i].m_pChannel)
						{
							it2.second[i].m_fPosition = GetPlaybackPosition(it2.second[i].m_pChannel);
						}
					}
				}

				if (!pSource->IsLooping())
				{
					pSource->StopIfEnded();
				}
				
				if (pSource->Is3DAudio())
				{
					Vector3 pos = pSource->GetTransform()->GetPosition();
					pSource->m_v3LastPosition = pSource->m_v3LastPosition == Vector3(-FLT_MAX) ? pos : pSource->m_v3LastPosition;

					Vector3 vel = pos - pSource->m_v3LastPosition;

					for (auto& it2 : pSource->m_pGroups)
					{
						for (uint32_t i = 0; i < it2.second.size(); ++i)
						{
							if (it2.second[i].m_pChannel)
							{
								m_pAudioContext->Set3DParameters(
									it2.second[i].m_pChannel,
									pos,
									vel
								);
							}
						}
					}

					pSource->m_v3LastPosition = pos;
				}
				else
				{
					for (auto& it2 : pSource->m_pGroups)
					{
						for (uint32_t i = 0; i < it2.second.size(); ++i)
						{
							if (it2.second[i].m_pChannel)
							{
								m_pAudioContext->Set3DParameters(
									it2.second[i].m_pChannel,
									pCamTransform->GetPosition(),
									Vector3(0.f)
								);
							}
						}
					}
				}
			}
		}
	}

	m_pAudioContext->Update();
}

void AudioSystem::OnWorldPaused(World* pWorld)
{
	for (auto& it : m_AudioSources)
	{
		if (it.first->IsEnabled()) {
			if (it.second == AS_PLAYING)
			{
				Pause(it.first, false);
			}
		}
	}
}

void AudioSystem::OnWorldResumed(World* pWorld)
{
	for (auto& it : m_AudioSources)
	{
		if (!it.first->IsEnabled())
			continue;

		if (it.second == AS_PLAYING)
			Play(it.first);
	}
}

void AudioSystem::Play(AudioSource* pSource, bool bApplyToSystem)
{
	if (!pSource->IsEnabled() || !pSource->GetSoundReference() || !m_pAudioContext)
		return;

	if (bApplyToSystem && m_AudioSources.find(pSource) != m_AudioSources.end())
		m_AudioSources[pSource] = AS_PLAYING;

	if (pSource->IsBGM())
	{
		m_pAudioContext->PlayBGM(pSource->GetSoundReference(), pSource->GetVolume(), pSource->GetLoopStart(), pSource->GetLoopEnd());
		return;
	}

	AudioSource::Group* pChannel = pSource->GetFreeChannel(pSource->m_pCurrentSound);

	if (pChannel)
	{
		m_pAudioContext->PlaySound(pSource->m_pCurrentSound, pChannel->m_pChannel, pSource->GetTransform()->GetPosition(), pSource->GetVolume(), false);

		m_pAudioContext->SetLoopCount(pChannel->m_pChannel, pSource->GetLoopCount());

		if (pSource->GetLoopEnd() > pSource->GetLoopStart())
			m_pAudioContext->SetLoopPoints(pChannel->m_pChannel, pSource->GetLoopStart(), pSource->GetLoopEnd());
	}
}

void AudioSystem::Pause(AudioSource* pSource, bool bApplyToSystem)
{
	if (bApplyToSystem && m_AudioSources.find(pSource) != m_AudioSources.end())
		m_AudioSources[pSource] = AS_PAUSED;

	for (auto& it : pSource->m_pGroups)
	{
		for (uint32_t i = 0; i < it.second.size(); ++i)
		{
			m_pAudioContext->PauseSound(it.second[i].m_pChannel);
		}
	}
}

void AudioSystem::Stop(AudioSource* pSource, bool bApplyToSystem)
{
	if (!m_pAudioContext)
		return;

	if (bApplyToSystem && m_AudioSources.find(pSource) != m_AudioSources.end())
		m_AudioSources[pSource] = AS_STOPPED;

	for (auto& it : pSource->m_pGroups)
	{
		for (uint32_t i = 0; i < it.second.size(); ++i)
		{
			m_pAudioContext->StopSound(it.second[i].m_pChannel);

			it.second[i].m_pChannel = nullptr;
			it.second[i].m_fPosition = 0.f;
		}
	}
}

bool AudioSystem::StopIfEnded(AudioSource* pSource, bool bApplyToSystem /*= true*/)
{
	if (!m_pAudioContext)
		return false;

	bool bAllEnded = true;

	for (auto& it : pSource->m_pGroups)
	{
		for (uint32_t i = 0; i < it.second.size(); ++i)
		{
			if (!it.second[i].m_pChannel)
				continue;

			float fPosition = pSource->GetPosition(it.first, i);

			if (fPosition != 0.0f)
			{
				int i = 9;
			}

			if (fPosition < 0 || fPosition < it.first->GetLength())
			{
				bAllEnded = false;
				continue;
			}

			m_pAudioContext->StopSound(it.second[i].m_pChannel);

			it.second[i].m_pChannel = nullptr;
			it.second[i].m_fPosition = 0.f;
		}
	}

	if (bAllEnded && bApplyToSystem && m_AudioSources.find(pSource) != m_AudioSources.end())
		m_AudioSources[pSource] = AS_STOPPED;

	return bAllEnded;
}

void AudioSystem::SetLoopCount(AudioSource* pSource)
{
	if (m_pAudioContext)
	{
		for (auto& it : pSource->m_pGroups)
		{
			for (uint32_t i = 0; i < it.second.size(); ++i)
			{
				m_pAudioContext->SetLoopCount(it.second[i].m_pChannel, pSource->IsLooping() ? pSource->GetLoopCount() : 0);
			}
		}
	}
}

void AudioSystem::SetLoopPoints(AudioSource* pSource)
{
	if (m_pAudioContext)
	{
		for (auto& it : pSource->m_pGroups)
		{
			for (uint32_t i = 0; i < it.second.size(); ++i)
			{
				m_pAudioContext->SetLoopPoints(it.second[i].m_pChannel, pSource->GetLoopStart(), pSource->GetLoopEnd());
			}
		}
	}
}

float AudioSystem::GetLength(AudioSource* pSource)
{
	if (m_pAudioContext)
		return m_pAudioContext->GetLength(pSource->m_pCurrentSound);

	return 0.0f;
}

float AudioSystem::GetPlaybackPosition(void* pChannel)
{
	if (m_pAudioContext)
		return m_pAudioContext->GetPlaybackPosition(pChannel);

	return 0.0f;
}

void AudioSystem::SetPlaybackPosition(AudioSource* pSource, SoundReference* pReference, void* pChannel)
{
	if (m_pAudioContext)
	{
		uint32_t i = 0;

		for (auto& it : pSource->m_pGroups)
		{
			for (i = 0; i < it.second.size(); ++i)
			{
				if (pChannel == it.second[i].m_pChannel)
					break;
			}
		}

		//m_pAudioContext->SetPlaybackPosition(pChannel, pSource->GetPosition(pReference, i));
	}
}

void AudioSystem::SetVolume(AudioSource* pSource)
{
	if (m_pAudioContext)
	{
		for (auto& it : pSource->m_pGroups)
		{
			for (uint32_t i = 0; i < it.second.size(); ++i)
			{
				m_pAudioContext->SetVolume(it.second[i].m_pChannel, m_fVolume * pSource->GetVolume());
			}
		}
	}
}

void AudioSystem::SetVolume(float fVolume)
{
	if (!m_pAudioContext)
		return;

	m_fVolume = std::max(0.0f, std::min(10.0f, fVolume));

	for (auto& it : m_AudioSources)
	{
		for (auto& it2 : it.first->m_pGroups)
		{
			for (uint32_t i = 0; i < it2.second.size(); ++i)
			{
				m_pAudioContext->SetVolume(it2.second[i].m_pChannel, m_fVolume * it.first->GetVolume());
			}
		}
	}
}

bool AudioSystem::IsPlaying(void* pChannel)
{
	return m_pAudioContext->IsPlaying(pChannel);
}

void AudioSystem::SetBGM(AudioSource* pSource, float m_fFadeDuration, bool bStopNext, bool bCrossfade)
{
	/* Switch to any previous pending BGM */
	if (m_pNextBGM) 
	{
		if (m_bBGMStopNext && m_pBGM)
			m_pBGM->Stop();
		else if(m_pBGM)
			m_pBGM->Pause();

		m_pBGM = m_pNextBGM;
		m_pBGM->Play();
	}

	/* Don't do anything */
	if (m_pBGM == pSource || m_pNextBGM == pSource)
		return;
	/* Immediately play BGM */
	else if (m_fFadeDuration <= 0)
	{
		if (m_pBGM)
		{
			if (bStopNext)
				m_pAudioContext->StopBGM();
			else
			{
				m_pAudioContext->PauseBGM();
				m_pBGM->SetPosition(m_pAudioContext->GetPlaybackPosition(m_pAudioContext->GetBGMChannel()));
			}
		}

		m_pBGM = pSource;

		m_pAudioContext->PlayBGM(m_pBGM->GetSoundReference(), m_pBGM->GetVolume(), m_pBGM->GetLoopStart(), m_pBGM->GetLoopEnd());
	}
	/* Use fading system */
	else
	{
		m_pNextBGM = pSource;

		m_fBGMMaxTimer = m_fFadeDuration;
		m_fBGMTimer = 0.0f;

		m_bBGMCrossFade = bCrossfade;
		m_bBGMStopNext = bStopNext;

		if (m_bBGMCrossFade)
		{
			m_pNextBGM->SetVolume(0.0f);
			m_pNextBGM->Play();
		}
	}
}

void AudioSystem::OnComponentAdded(Component* pComponent)
{
	bool bIsEditor = m_pWorld->GetApplication()->IsInEditor();

	if (AudioSource* pSource = dynamic_cast<AudioSource*>(pComponent))
	{
		m_AudioSources[pSource] = pSource->GetState();

		if (!bIsEditor)
		{
			if (pSource->IsBGM() && pSource->m_bAutoPlay)
				pSource->SetAsBGM(0.0f);
			else if (pSource->m_bAutoPlay)
				pSource->Play();
		}
	}
}

void AudioSystem::OnComponentDestroyed(Component* pComponent)
{
	if (AudioSource* pSource = dynamic_cast<AudioSource*>(pComponent))
	{
		if (!pSource->GetFilePath().empty())
		{
			Stop(pSource);

			for (auto& it : pSource->m_pGroups)
			{
				for (uint32_t i = 0; i < it.second.size(); ++i)
				{
					it.second[i].m_pChannel = nullptr;
				}
			}

			pSource->m_pCurrentSound = nullptr;
		}

		auto find = m_AudioSources.find(pSource);

		if (find != m_AudioSources.end())
		{
			m_AudioSources.erase(find);
		}
	}
}