#pragma once

#include "Core/ECS/Component.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Core/Resources/Formats/SoundReference.h"

#include <External/rttr/type>

class AudioSystem;

enum AudioState {
	AS_PLAYING,
	AS_PAUSED,
	AS_STOPPED
};

class AudioSource : public Component
{
	friend class AudioSystem;

public:
	AudioSource(Entity* pOwner);
	~AudioSource();

	void Awake() override;
	void Start() override;

	void Play();
	void Pause();
	void Stop();

	void StopIfEnded();

	AudioState GetState() const { return m_State; }

	bool IsPlaying() const { return m_State == AS_PLAYING; }
	void SetPlaying(bool bPlaying);

	bool IsLooping() const { return m_bIsLooping; }
	void SetLooping(bool bLooping) { m_bIsLooping = bLooping; }

	int32_t GetLoopCount() const { return m_iLoopCount; }
	void SetLoopCount(int32_t iLoopCount);

	uint32_t GetLoopStart() const { return m_uiLoopStart; };
	void SetLoopStart(uint32_t uiLoopPoint);

	uint32_t GetLoopEnd() const { return m_uiLoopEnd; };
	void SetLoopEnd(uint32_t uiLoopPoint);

	void SetLoopPoints(uint32_t uiLoopStart, uint32_t uiLoopEnd);

	const std::string& GetFilePath() const { return m_FilePath; }
	void SetFilePath(const std::string& filepath);

	SoundReference* GetSoundReference() const { return m_pCurrentSound; }
	void SetSoundReference(SoundReference* pReference, bool bForceSet = false, bool bIncrementRef = true);

	float GetLength() const;

	float GetPosition(SoundReference* pReference, uint32_t i) { return m_pGroups[pReference][i].m_fPosition; };
	void SetPosition(float fPosition);

	float GetPositionNorm();
	void SetPositionNorm(float fPosition);

	float GetVolume() const { return m_fVolume; };
	void SetVolume(float fVolume);

	bool Is3DAudio() { return m_bIs3DAudio; };
	void Set3DAudio(bool bIs3D);

	bool IsBGM() { return m_bIsBGM; };
	void SetBGM(bool bIsBGM);

	void SetAsBGM(float fFadeDuration = 0.5f, bool bStopNext = false, bool bCrossfade = false);

	bool m_bAutoPlay = false;

private:
	struct Group
	{
		float m_fPosition = 0.0f;
		void* m_pChannel = nullptr;
	};

	virtual void OnEnabled() override;
	virtual void OnDisabled() override;

	Group* GetFreeChannel(SoundReference* pReference);

	std::string m_FilePath = "";

	AudioState m_State = AS_STOPPED;

	AudioSystem* m_pAudioSystem = nullptr;

	uint32_t m_uiLoopStart = 0;
	uint32_t m_uiLoopEnd = 0;

	float m_fVolume = 1.0f;

	bool m_bIs3DAudio = true;

	int32_t m_iLoopCount = 0;
	bool m_bIsLooping = false;

	bool m_bIsBGM = false;

	Vector3 m_v3LastPosition = Vector3(-FLT_MAX);

	std::unordered_map<SoundReference*, std::vector<Group>> m_pGroups;

	SoundReference* m_pCurrentSound = nullptr;
	Group* m_pCurrentGroup = nullptr;

	RTTR_ENABLE(Component)
};