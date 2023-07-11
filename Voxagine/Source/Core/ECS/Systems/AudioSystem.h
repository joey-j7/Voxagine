#pragma once

#include "Core/ECS/ComponentSystem.h"
#include "Core/ECS/Components/BehaviorScript.h"

#include "../Components/AudioSource.h"

class AudioContext;

class AudioSystem : public ComponentSystem
{
	friend class AudioSource;

public:
	AudioSystem(World* pWorld);
	virtual ~AudioSystem();

	virtual void Start();
	virtual bool CanProcessComponent(Component* pComponent) override;

	virtual void Tick(float fDeltaTime) override;
	virtual void PostTick(float fDeltaTime) override;

	float GetVolume() const { return m_fVolume; }
	void SetVolume(float fVolume);

	bool IsPlaying(void* pChannel);

protected:
	void OnWorldPaused(World* pWorld);
	void OnWorldResumed(World* pWorld);

	void Play(AudioSource* pSource, bool bApplyToSystem = true);
	void Pause(AudioSource* pSource, bool bApplyToSystem = true);
	void Stop(AudioSource* pSource, bool bApplyToSystem = true);
	
	bool StopIfEnded(AudioSource* pSource, bool bApplyToSystem = true);

	void SetLoopCount(AudioSource* pSource);
	void SetLoopPoints(AudioSource* pSource);

	float GetLength(AudioSource* pSource);

	float GetPlaybackPosition(void* pChannel);
	void SetPlaybackPosition(AudioSource* pSource, SoundReference* pReference, void* pChannel);

	void SetVolume(AudioSource* pSource);

	void SetBGM(AudioSource* pSource, float m_fFadeoutDuration = 0.5f, bool bStopNext = false, bool bCrossfade = false);

	virtual void OnComponentAdded(Component* pComponent) override;
	virtual void OnComponentDestroyed(Component* pComponent) override;

	AudioContext* m_pAudioContext = nullptr;
	std::unordered_map<AudioSource*, AudioState> m_AudioSources;

	Vector3 m_v3LastCameraPosition = Vector3(-FLT_MAX);

	AudioSource* m_pBGM = nullptr;
	AudioSource* m_pNextBGM = nullptr;

	float m_fVolume = 1.0f;

	float m_fBGMTimer = 0.0f;
	float m_fBGMMaxTimer = 0.0f;

	bool m_bBGMStopNext = false;
	bool m_bBGMCrossFade = false;
};