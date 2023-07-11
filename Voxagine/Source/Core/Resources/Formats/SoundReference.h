#pragma once

#include "Core/Resources/ReferenceObject.h"

class AudioContext;
class SoundReference : public ReferenceObject
{
	friend class ResourceManager;
	friend class AudioSource;

public:
	SoundReference(const std::string& filePath) : ReferenceObject(filePath) {}
	virtual ~SoundReference() {}

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