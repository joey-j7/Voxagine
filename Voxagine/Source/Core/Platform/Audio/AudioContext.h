#pragma once

#include "Core/Math.h"
#include <string>
#include <algorithm>

class Platform;
class LoggingSystem;
class SoundReference;

class AudioContext
{
public:
	AudioContext(Platform* pPlatform);
	virtual ~AudioContext() {}

	virtual void Initialize() = 0;
	virtual void Update() = 0;

	virtual bool CreateSound(const std::string& soundPath, void*& pSound, bool bIs3D = true) = 0;

	virtual void PlaySound(const SoundReference* pSoundReference, void*& pChannel, const Vector3& v3Position = Vector3(0.f), float fVolume = 1.0f, bool bIsPaused = false) = 0;
	virtual void PauseSound(void* pChannel) = 0;
	virtual void StopSound(void* pChannel) = 0;

	virtual void PlayBGM(SoundReference* pSoundReference, float fVolume, uint32_t uiLoopStart, uint32_t uiLoopEnd) = 0;
	virtual void ResumeBGM() = 0;
	virtual void PauseBGM() = 0;
	virtual void StopBGM() = 0;

	float GetBGMVolume() const { return m_fBGMVolume; };
	virtual void SetBGMVolume(float fVolume) = 0;

	virtual bool IsBGMPlaying() const = 0;

	virtual void* GetBGMChannel() = 0;
	virtual SoundReference* GetBGMReference() const { return m_pBGMReference; };

	virtual bool IsPlaying(void* pChannel) { return false; };

	virtual float GetLength(const SoundReference* pSoundReference) = 0;
	virtual float GetPlaybackPosition(void* pChannel) = 0;

	virtual float GetVolume(void* pChannel) const = 0;
	virtual void SetVolume(void* pChannel, float fVolume) = 0;

	virtual void SetPlaybackPosition(void* pChannel, float position) = 0;

	virtual void Set3DSystemParameters(const Vector3& v3Position, const Vector3& v3Velocity, const Vector3& v3Forward, const Vector3& v3Up) = 0;
	virtual void Set3DParameters(void* pChannel, Vector3 position, Vector3 velocity = Vector3(0.f)) = 0;

	virtual void GetLoopPoints(void* pChannel, uint32_t& uiLoopStart, uint32_t& uiLoopEnd) const = 0;
	virtual void SetLoopPoints(void* pChannel, uint32_t uiLoopStart, uint32_t uiLoopEnd) = 0;

	virtual void SetLoopCount(void* pChannel, int32_t iLoopCount) = 0;

protected:
	Platform* m_pPlatform = nullptr;
	LoggingSystem* m_pLoggingSystem = nullptr;

	float m_fBGMVolume = 1.f;

	SoundReference* m_pBGMReference = nullptr;
};