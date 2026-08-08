#pragma once

#include "Core/Platform/Audio/AudioContext.h"

/* Silent AudioContext.
 *
 * FMOD ships as a proprietary SDK that has to be downloaded by hand, so it is
 * off by default and this stands in. Everything that plays a sound keeps
 * working; nothing is audible.
 *
 * Channel handles are the SoundReference pointer itself rather than null, so
 * callers that compare or store handles still behave sensibly, and
 * IsPlaying() reports false so anything waiting on playback finishing does not
 * hang. */
class NullAudioContext : public AudioContext
{
public:
	NullAudioContext(Platform* pPlatform);

	virtual void Initialize() override;
	virtual void Update() override;

	virtual bool CreateSound(const std::string& soundPath, void*& pSound, bool bIs3D = true) override;

	virtual void PlaySound(const SoundReference* pSoundReference, void*& pChannel,
	                       const Vector3& v3Position = Vector3(0.f), float fVolume = 1.0f,
	                       bool bIsPaused = false) override;

	virtual void PauseSound(void* pChannel) override;
	virtual void StopSound(void* pChannel) override;

	virtual void PlayBGM(SoundReference* pSoundReference, float fVolume,
	                     uint32_t uiLoopStart, uint32_t uiLoopEnd) override;

	virtual void ResumeBGM() override;
	virtual void PauseBGM() override;
	virtual void StopBGM() override;

	virtual void SetBGMVolume(float fVolume) override;
	virtual bool IsBGMPlaying() const override;

	virtual void* GetBGMChannel() override;

	virtual bool IsPlaying(void* pChannel) override;

	virtual float GetLength(const SoundReference* pSoundReference) override;
	virtual float GetPlaybackPosition(void* pChannel) override;

	virtual float GetVolume(void* pChannel) const override;
	virtual void SetVolume(void* pChannel, float fVolume) override;

	virtual void SetPlaybackPosition(void* pChannel, float position) override;

	virtual void Set3DSystemParameters(const Vector3& v3Position, const Vector3& v3Velocity,
	                                   const Vector3& v3Forward, const Vector3& v3Up) override;

	virtual void Set3DParameters(void* pChannel, Vector3 position,
	                             Vector3 velocity = Vector3(0.f)) override;

	virtual void GetLoopPoints(void* pChannel, uint32_t& uiLoopStart, uint32_t& uiLoopEnd) const override;
	virtual void SetLoopPoints(void* pChannel, uint32_t uiLoopStart, uint32_t uiLoopEnd) override;

	virtual void SetLoopCount(void* pChannel, int32_t iLoopCount) override;

private:
	bool m_bBGMPlaying = false;
};
