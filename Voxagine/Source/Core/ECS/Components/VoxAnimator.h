#pragma once

#include "Core/ECS/Component.h"
#include <External/rttr/type>

#include <unordered_map>

class VoxModel;
struct VoxFrame;
class VoxRenderer;

class AudioSource;
class AudioPlaylist;

class VoxAnimator : public Component
{
public:
	VoxAnimator(Entity* pOwner);
	virtual ~VoxAnimator();

	virtual void OnEnabled() override;
	virtual void OnDisabled() override;

	virtual void Awake() override;
	virtual void Start() override;

	void Tick(float fDeltaTime);

	std::string GetAnimModelFilePath() const;
	void SetAnimModelFilePath(std::string filePath);

	void SetAnimModel(VoxModel* pModel);

	uint32_t GetFPS() const { return m_FPS; }
	void SetFPS(uint32_t uiFPS) { m_FPS = uiFPS; }

	float GetSpeed() const { return m_fSpeed; }
	void SetSpeed(float fSpeed) { m_fSpeed = std::max(fSpeed, 0.0f); }

	void Play() { 
		m_bIsPaused = false; 
		m_bIsStopped = false;
	};
	bool IsPlaying() const { return !m_bIsPaused && !m_bIsStopped; }

	void Pause() { m_bIsPaused = true; };
	bool IsPaused() const { return m_bIsPaused; }

	void Stop() {
		m_bIsPaused = false;
		m_bIsStopped = true;
		m_fFrameTimer = 0.0f;
	};

	bool IsStopped() const { return m_bIsStopped; }

	uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
	void SetCurrentFrameIndex(uint32_t uiCurrentFrame) { m_CurrentFrame = uiCurrentFrame; }

	const VoxFrame* GetCurrentFrame() { return m_pCurrentFrame; };

	void SetCurrentAnimationFile(std::string filePath);
	bool HasAnimation(std::string filePath);
	unsigned int GetAnimationIndex(std::string filePath);

	void SetCurrentAnimation(unsigned int currentAnimationIndex);
	void SetCurrentAnimationWithFrame(unsigned int currentAnimationIndex, unsigned int currentFrame);
	unsigned int GetCurrentAnimation();

	void ResetAnimationTimer();

	void SetAnimationFiles(std::vector<std::string>& animationFiles);
	std::vector<std::string>& GetAnimationFiles();
private:
	void SetAnimationFrame(unsigned int newAnimationFrame);
	
	VoxModel* GetActiveAnimationModel();

	void AddAnimationFile(std::string animationFile, unsigned int animationFileIndex);
	void RemoveAnimationFile(unsigned int animationFileIndex);
	void ClearAnimationFiles();

	void CheckCurrentAnimation();
	void SetToCurrentAnimation(unsigned int iResetAnimFrame = 0);
private:
	VoxRenderer* m_pVoxRenderer = nullptr;

	AudioSource* m_pAudioSource = nullptr;
	AudioPlaylist* m_pAudioPlaylist = nullptr;

	std::vector<std::string> m_AnimationFiles;
	std::vector<VoxModel*> m_AnimationModels;
	
	bool m_bHasAudio = false;

	unsigned int m_CurrentAnimationIndex = 0;
	unsigned int m_CurrentFrame = 0;
	unsigned int m_FPS = 10;

	float m_fSpeed = 1.0f;
	float m_fFrameTimer = 0.0f;

	// Below everything is not needed anymore
	VoxModel* m_pModel = nullptr;
	const VoxFrame* m_pCurrentFrame = nullptr;


	bool m_bIsPaused = false;
	bool m_bIsStopped = false;

	RTTR_ENABLE(Component)
};