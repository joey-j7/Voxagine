#pragma once

#include "Core/Platform/Audio/AudioContext.h"
#include "Core/Resources/ReferenceObject.h"

class SoundReference : public ReferenceObject
{
	friend class ResourceManager;
	friend class AudioSource;

public:
	SoundReference(const std::string& filePath) : ReferenceObject(filePath) {}

	virtual ~SoundReference()
	{
		/* The context may still be pointing at this as the playing BGM. */
		if (m_pAudioContext != nullptr)
			m_pAudioContext->OnReferenceDestroyed(this);
	}

	void SetContext(AudioContext* pContext) { m_pAudioContext = pContext; }

	virtual bool Load(const std::string& filePath) = 0;
	virtual void Free() = 0;

	uint32_t GetLoopStart() const { return m_uiLoopStart; }
	uint32_t GetLoopEnd() const { return m_uiLoopEnd; }

	float GetLength() const { return m_fLength; }

	void* Sound = nullptr;

protected:
	AudioContext* m_pAudioContext;

	uint32_t m_uiLoopStart = 0;
	uint32_t m_uiLoopEnd = 0;

	float m_fLength = 0.0f;
};