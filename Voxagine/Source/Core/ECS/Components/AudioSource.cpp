#include "pch.h"
#include "AudioSource.h"

#include "Core/ECS/World.h"
#include "Core/ECS/Systems/AudioSystem.h"

#include <External/rttr/registration.h>
#include "Core/MetaData/PropertyTypeMetaData.h"
#include "Core/Application.h"
#include "../ComponentSystem.h"
#include "Core/LoggingSystem/LoggingSystem.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<AudioSource>("AudioSource")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("File", &AudioSource::GetFilePath, &AudioSource::SetFilePath) ( RTTR_PUBLIC, RTTR_RESOURCE("ogg") )
		.property("Looping", &AudioSource::IsLooping, &AudioSource::SetLooping) (RTTR_PUBLIC)
		.property("Loop Count", &AudioSource::GetLoopCount, &AudioSource::SetLoopCount) (RTTR_PUBLIC)
		.property_readonly("Loop Start", &AudioSource::GetLoopStart) (RTTR_PUBLIC)
		.property_readonly("Loop End", &AudioSource::GetLoopEnd) (RTTR_PUBLIC)
		.property("Volume", &AudioSource::GetVolume, &AudioSource::SetVolume) (RTTR_PUBLIC)
		.property("Play", &AudioSource::IsPlaying, &AudioSource::SetPlaying) (RTTR_PUBLIC)
		.property("Playback Position", &AudioSource::GetPositionNorm, &AudioSource::SetPositionNorm) (RTTR_PUBLIC)
		.property("3D Audio", &AudioSource::Is3DAudio, &AudioSource::Set3DAudio) (RTTR_PUBLIC)
		.property("Auto Play", &AudioSource::m_bAutoPlay) (RTTR_PUBLIC)
		.property("Is BGM", &AudioSource::IsBGM, &AudioSource::SetBGM) (RTTR_PUBLIC);
}

AudioSource::AudioSource(Entity* pOwner)
	: Component(pOwner)
{
	m_pAudioSystem = pOwner->GetWorld()->GetSystem<AudioSystem>();
}

AudioSource::~AudioSource()
{
	Stop();

	m_FilePath = "";

	for (auto& it : m_pGroups)
	{
		it.first->Release();
	}

	m_pGroups.clear();
}

void AudioSource::Awake()
{
	Component::Awake();
	m_pAudioSystem = GetWorld()->GetSystem<AudioSystem>();
}

void AudioSource::Start()
{
	Component::Start();
}

void AudioSource::Play()
{
	m_State = AS_PLAYING;

	if (m_pAudioSystem)
	{
		m_pAudioSystem->Play(this);
	}
}

void AudioSource::Pause()
{
	m_State = AS_PAUSED;

	if (m_pAudioSystem)
		m_pAudioSystem->Pause(this);
}

void AudioSource::Stop()
{
	m_State = AS_STOPPED;

	if (m_pAudioSystem)
		m_pAudioSystem->Stop(this);
}

void AudioSource::StopIfEnded()
{
	if (m_pAudioSystem)
	{
		if (m_pAudioSystem->StopIfEnded(this))
		{
			m_State = AS_STOPPED;
		}
	}
}

void AudioSource::SetPlaying(bool bPlaying)
{
	if (m_pAudioSystem)
	{
		m_State = bPlaying ? AS_PLAYING : AS_STOPPED;

		if (m_State == AS_PLAYING)
		{
			m_pAudioSystem->Play(this);
		}
		else
		{
			m_pAudioSystem->Stop(this);
		}
	}
}

void AudioSource::SetLoopCount(int32_t iLoopCount)
{
	m_iLoopCount = std::max(-1, iLoopCount);

	if (m_pAudioSystem)
		m_pAudioSystem->SetLoopCount(this);
}

void AudioSource::SetLoopStart(uint32_t uiLoopPoint)
{
	m_uiLoopStart = uiLoopPoint;
	m_uiLoopEnd = std::max(m_uiLoopStart, m_uiLoopEnd);

	if (m_pAudioSystem)
		m_pAudioSystem->SetLoopPoints(this);
}

void AudioSource::SetLoopEnd(uint32_t uiLoopPoint)
{
	m_uiLoopEnd = uiLoopPoint;
	m_uiLoopStart = std::min(m_uiLoopStart, m_uiLoopEnd);

	if (m_pAudioSystem)
		m_pAudioSystem->SetLoopPoints(this);
}

void AudioSource::SetLoopPoints(uint32_t uiLoopStart, uint32_t uiLoopEnd)
{
	m_uiLoopStart = std::min(uiLoopStart, uiLoopEnd);
	m_uiLoopEnd = std::max(uiLoopStart, uiLoopEnd);

	if (m_uiLoopStart == m_uiLoopEnd)
	{
		m_uiLoopStart = 0;
		m_uiLoopEnd = GetLength();

		if (m_uiLoopEnd == 0)
			return;
	}

	if (m_pAudioSystem)
		m_pAudioSystem->SetLoopPoints(this);
}

void AudioSource::SetFilePath(const std::string& filepath)
{
	if (filepath.empty()) return;

	m_FilePath = filepath;

	SoundReference* pReference = GetWorld()->GetApplication()->GetResourceManager().LoadSound(filepath);

	// Delete reference once as it already exists (twice in memory at this point)
	if (m_pGroups.find(pReference) != m_pGroups.end())
	{
		pReference->Release();
	}

	GetFreeChannel(pReference);
	SetLoopPoints(pReference->GetLoopStart(), pReference->GetLoopEnd());
}

void AudioSource::SetSoundReference(SoundReference* pReference, bool bForceSet, bool bIncrementRef)
{
	GetFreeChannel(pReference);
	SetLoopPoints(pReference->GetLoopStart(), pReference->GetLoopEnd());

	if (bIncrementRef && pReference)
	{
		pReference->IncrementRef();
	}

	m_FilePath = pReference ? pReference->GetRefPath() : "";
}

float AudioSource::GetLength() const
{
	if (!m_pCurrentSound)
		return 0.0f;

	return m_pCurrentSound->m_fLength;
}

void AudioSource::SetPosition(float fPosition)
{
	if (m_pGroups.empty())
		return;

	auto& group = m_pGroups[m_pCurrentSound];

	for (uint32_t i = 0; i < group.size(); ++i)
	{
		group[i].m_fPosition = glm::clamp(fPosition, 0.0f, GetLength());
	}

	if (m_pAudioSystem)
		m_pAudioSystem->SetPlaybackPosition(this, m_pCurrentSound, m_pCurrentGroup);
}

float AudioSource::GetPositionNorm()
{
	if (GetLength() <= 0 || m_pGroups.empty())
		return 0.0f;

	return m_pCurrentGroup->m_fPosition / GetLength();
}

void AudioSource::SetPositionNorm(float fPosition)
{
	if (m_pGroups.empty() || !m_pCurrentSound)
		return;

	auto& group = m_pGroups[m_pCurrentSound];

	for (uint32_t i = 0; i < group.size(); ++i)
	{
		group[i].m_fPosition = glm::clamp(fPosition, 0.0f, 1.0f) * GetLength();
	}

	if (m_pAudioSystem)
		m_pAudioSystem->SetPlaybackPosition(this, m_pCurrentSound, m_pCurrentGroup->m_pChannel);
}

void AudioSource::SetVolume(float fVolume)
{
	m_fVolume = std::max(0.0f, std::min(10.0f, fVolume));

	if (m_pAudioSystem)
		m_pAudioSystem->SetVolume(this);
}

void AudioSource::Set3DAudio(bool bIs3D)
{
	if (m_bIs3DAudio == bIs3D)
		return;

	m_bIs3DAudio = bIs3D;

	if (bIs3D)
		SetBGM(false);
}

void AudioSource::SetBGM(bool bIsBGM)
{
	if (m_bIsBGM == bIsBGM)
		return;

	m_bIsBGM = bIsBGM;
	Stop();

	if (bIsBGM)
		m_bIs3DAudio = false;
}

void AudioSource::SetAsBGM(float fFadeDuration, bool bStopNext, bool bCrossfade)
{
	std::vector<Entity*> pEntities = GetWorld()->FindEntitiesWithComponent<AudioSource>();

	SetBGM(true);

	if (m_pAudioSystem)
		m_pAudioSystem->SetBGM(this, fFadeDuration, bStopNext, bCrossfade);
}

void AudioSource::OnEnabled()
{
	if (m_pAudioSystem && m_State == AS_PLAYING)
		m_pAudioSystem->Play(this);
}

void AudioSource::OnDisabled()
{
	if (m_pAudioSystem)
		m_pAudioSystem->Pause(this, false);
}

AudioSource::Group* AudioSource::GetFreeChannel(SoundReference* pReference)
{
	if (!pReference)
		return nullptr;

	auto it = m_pGroups.find(pReference);
	if (it == m_pGroups.end()) m_pGroups.emplace(pReference, std::vector<Group>{});

	for (uint32_t i = 0; i < m_pGroups[pReference].size(); ++i)
	{
		if (!m_pGroups[pReference][i].m_pChannel)
		{
			m_pCurrentSound = pReference;
			m_pCurrentGroup = &m_pGroups[pReference][i];

			return m_pCurrentGroup;
		}
	}

	if (m_pAudioSystem)
		m_pAudioSystem->m_pWorld->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_MESSAGE, "Audio", std::to_string(m_pGroups.size() + 1).c_str() + ':' + pReference->GetRefPath());

	m_pGroups[pReference].push_back(Group());

	m_pCurrentSound = pReference;
	m_pCurrentGroup = &m_pGroups[pReference].back();

	return m_pCurrentGroup;
}