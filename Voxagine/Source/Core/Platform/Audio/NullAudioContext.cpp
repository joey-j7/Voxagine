#include "pch.h"
#include "NullAudioContext.h"

#include "Core/Resources/Formats/SoundReference.h"

#include <cstdio>

NullAudioContext::NullAudioContext(Platform* pPlatform) : AudioContext(pPlatform)
{
}

void NullAudioContext::Initialize()
{
	printf("[audio] running silent; build with VOXAGINE_ENABLE_FMOD for sound\n");
}

void NullAudioContext::Update()
{
}

bool NullAudioContext::CreateSound(const std::string& soundPath, void*& pSound, bool bIs3D)
{
	VX_UNUSED(soundPath);
	VX_UNUSED(bIs3D);

	/* Non-null so callers treat the sound as loaded. */
	pSound = this;

	return true;
}

void NullAudioContext::PlaySound(const SoundReference* pSoundReference, void*& pChannel,
                                 const Vector3& v3Position, float fVolume, bool bIsPaused)
{
	VX_UNUSED(v3Position);
	VX_UNUSED(fVolume);
	VX_UNUSED(bIsPaused);

	pChannel = const_cast<SoundReference*>(pSoundReference);
}

void NullAudioContext::PauseSound(void* pChannel) { VX_UNUSED(pChannel); }
void NullAudioContext::StopSound(void* pChannel) { VX_UNUSED(pChannel); }

void NullAudioContext::PlayBGM(SoundReference* pSoundReference, float fVolume,
                               uint32_t uiLoopStart, uint32_t uiLoopEnd)
{
	VX_UNUSED(uiLoopStart);
	VX_UNUSED(uiLoopEnd);

	m_pBGMReference = pSoundReference;
	m_fBGMVolume = fVolume;
	m_bBGMPlaying = pSoundReference != nullptr;
}

void NullAudioContext::ResumeBGM() { m_bBGMPlaying = m_pBGMReference != nullptr; }
void NullAudioContext::PauseBGM() { m_bBGMPlaying = false; }

void NullAudioContext::StopBGM()
{
	m_bBGMPlaying = false;
	m_pBGMReference = nullptr;
}

void NullAudioContext::SetBGMVolume(float fVolume) { m_fBGMVolume = fVolume; }
bool NullAudioContext::IsBGMPlaying() const { return m_bBGMPlaying; }

void* NullAudioContext::GetBGMChannel() { return m_pBGMReference; }

bool NullAudioContext::IsPlaying(void* pChannel)
{
	VX_UNUSED(pChannel);

	/* False, so anything that waits for a sound to finish proceeds. */
	return false;
}

float NullAudioContext::GetLength(const SoundReference* pSoundReference)
{
	VX_UNUSED(pSoundReference);
	return 0.f;
}

float NullAudioContext::GetPlaybackPosition(void* pChannel)
{
	VX_UNUSED(pChannel);
	return 0.f;
}

float NullAudioContext::GetVolume(void* pChannel) const
{
	VX_UNUSED(pChannel);
	return 0.f;
}

void NullAudioContext::SetVolume(void* pChannel, float fVolume)
{
	VX_UNUSED(pChannel);
	VX_UNUSED(fVolume);
}

void NullAudioContext::SetPlaybackPosition(void* pChannel, float position)
{
	VX_UNUSED(pChannel);
	VX_UNUSED(position);
}

void NullAudioContext::Set3DSystemParameters(const Vector3& v3Position, const Vector3& v3Velocity,
                                             const Vector3& v3Forward, const Vector3& v3Up)
{
	VX_UNUSED(v3Position);
	VX_UNUSED(v3Velocity);
	VX_UNUSED(v3Forward);
	VX_UNUSED(v3Up);
}

void NullAudioContext::Set3DParameters(void* pChannel, Vector3 position, Vector3 velocity)
{
	VX_UNUSED(pChannel);
	VX_UNUSED(position);
	VX_UNUSED(velocity);
}

void NullAudioContext::GetLoopPoints(void* pChannel, uint32_t& uiLoopStart, uint32_t& uiLoopEnd) const
{
	VX_UNUSED(pChannel);

	uiLoopStart = 0;
	uiLoopEnd = 0;
}

void NullAudioContext::SetLoopPoints(void* pChannel, uint32_t uiLoopStart, uint32_t uiLoopEnd)
{
	VX_UNUSED(pChannel);
	VX_UNUSED(uiLoopStart);
	VX_UNUSED(uiLoopEnd);
}

void NullAudioContext::SetLoopCount(void* pChannel, int32_t iLoopCount)
{
	VX_UNUSED(pChannel);
	VX_UNUSED(iLoopCount);
}
