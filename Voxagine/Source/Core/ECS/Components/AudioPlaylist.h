#pragma once

#include "Core/ECS/Component.h"
#include <External/rttr/type>

#include <unordered_map>

class AudioSource;
class SoundReference;

class AudioPlaylist : public Component
{
public:
	friend class VoxAnimator;

	AudioPlaylist(Entity* pOwner);
	virtual ~AudioPlaylist();

	virtual void OnEnabled() override;
	virtual void OnDisabled() override;

	virtual void Awake() override;

	void Play(uint32_t uiIndex, bool bForce = false);
	void PlayNext();

	uint32_t GetCurrentIndex() const { return m_uiCurrentIndex; }
	void SetCurrentIndex(uint32_t uiIndex);

private:
	void AddAudioFile(SoundReference* audioFile, unsigned int audioFileIndex);
	void RemoveAudioFile(unsigned int audioFileIndex);
	void ClearAudioFiles();

	SoundReference* GetActiveSoundReference() const;

	AudioSource* m_pAudioSource = nullptr;

	std::vector<SoundReference*> m_SoundReferences;

	uint32_t m_uiCurrentIndex = 0;

	RTTR_ENABLE(Component)
};