#include "pch.h"
#include "VoxAnimator.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "Core/Resources/Formats/VoxModel.h"
#include "VoxRenderer.h"

#include "Core/ECS/World.h"
#include "Core/Application.h"

#include "AudioPlaylist.h"
#include "AudioSource.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<VoxAnimator>("VoxAnimator")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
	/*
			.property("Play", &VoxAnimator::IsPlaying, &VoxAnimator::Play)
			.property("Pause", &VoxAnimator::IsPaused, &VoxAnimator::Pause)
			.property("Stop", &VoxAnimator::IsStopped, &VoxAnimator::Stop)
	*/
		.property("File", &VoxAnimator::GetAnimModelFilePath, &VoxAnimator::SetAnimModelFilePath) (RTTR_PUBLIC, RTTR_RESOURCE("anim.vox"))
		.property("Current Frame", &VoxAnimator::GetCurrentFrameIndex, &VoxAnimator::SetCurrentFrameIndex) (RTTR_PUBLIC)
		.property("Frames per Second", &VoxAnimator::GetFPS, &VoxAnimator::SetFPS) (RTTR_PUBLIC)
		.property("Current Animation Index", &VoxAnimator::GetCurrentAnimation, &VoxAnimator::SetCurrentAnimation) (RTTR_PUBLIC)
		.property("Test animations", &VoxAnimator::GetAnimationFiles, &VoxAnimator::SetAnimationFiles) (RTTR_PUBLIC, RTTR_RESOURCE("anim.vox"));
}

VoxAnimator::VoxAnimator(Entity* pOwner) :
	Component(pOwner)
{
	Requires<VoxRenderer>();
}

VoxAnimator::~VoxAnimator()
{
	ClearAnimationFiles();
}

void VoxAnimator::OnEnabled()
{
	m_pVoxRenderer = GetOwner()->GetComponent<VoxRenderer>();
}

void VoxAnimator::OnDisabled()
{
	m_pVoxRenderer = nullptr;
}

void VoxAnimator::Awake()
{
	Component::Awake();

	m_pVoxRenderer = GetOwner()->GetComponent<VoxRenderer>();
}

void VoxAnimator::Start()
{
	Component::Start();

	std::string filePath = GetAnimModelFilePath();

	if (!filePath.empty())
	{
		SetAnimModelFilePath("");
		SetAnimModelFilePath(filePath);

		SetAnimationFrame(0);
	}
}

void VoxAnimator::Tick(float fDeltaTime)
{
 	if (m_bIsPaused || m_bIsStopped)
 		return;

	//CheckCurrentAnimation();

	VoxModel* pCurrentAnimationModel = GetActiveAnimationModel();

	if (pCurrentAnimationModel != nullptr)
	{
		m_fFrameTimer += fDeltaTime * m_FPS * m_fSpeed;

		/* When it's time to show the next frame */
		if (m_fFrameTimer >= 1.0f && pCurrentAnimationModel->GetFrameCount() > 0)
		{
			/* Append number of frames depending on timer */
			m_CurrentFrame = (m_CurrentFrame + static_cast<uint32_t>(m_fFrameTimer)) % pCurrentAnimationModel->GetFrameCount();

			/* Clamp timer */
			m_fFrameTimer = fmod(m_fFrameTimer, 1.0f);

			/* Set current frame */
			SetAnimationFrame(m_CurrentFrame);
		}
	}
}

std::string VoxAnimator::GetAnimModelFilePath() const
{
	return m_pModel ? m_pModel->GetRefPath() : "";
}

void VoxAnimator::SetAnimModelFilePath(std::string filePath)
{
	if (m_pModel && filePath == m_pModel->GetRefPath())
	{
		return;
	}

	if (!filePath.empty())
	{
		if (m_pModel)
			m_pModel->Release();

		VoxModel* pModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox(filePath);
		SetAnimModel(pModel);
	}
	else
	{
		if (m_pVoxRenderer)
			m_pVoxRenderer->ResetModel();

		SetAnimModel(nullptr);
	}
}

void VoxAnimator::SetAnimModel(VoxModel* pModel)
{
	m_pModel = pModel;
}

void VoxAnimator::SetCurrentAnimationFile(std::string filePath)
{
	unsigned int AnimationIndex = GetAnimationIndex(filePath);
	SetCurrentAnimation(AnimationIndex);
}

bool VoxAnimator::HasAnimation(std::string filePath)
{
	return (GetAnimationIndex(filePath) != -1);
}

unsigned int VoxAnimator::GetAnimationIndex(std::string filePath)
{
	for (unsigned int animationIndexIt = 0; animationIndexIt != m_AnimationFiles.size(); ++animationIndexIt)
	{
		if (m_AnimationFiles[animationIndexIt] == filePath)
			return animationIndexIt;
	}

	return UINT_MAX;
}

void VoxAnimator::SetCurrentAnimation(unsigned int currentAnimationIndex)
{
	SetCurrentAnimationWithFrame(currentAnimationIndex, 0);
}

void VoxAnimator::SetCurrentAnimationWithFrame(unsigned int currentAnimationIndex, unsigned int currentFrame)
{
	m_CurrentAnimationIndex = currentAnimationIndex;
	m_CurrentFrame = currentFrame;

	if (GetActiveAnimationModel() != nullptr)
		SetToCurrentAnimation(m_CurrentFrame);
	else
	{
		if (m_pVoxRenderer != nullptr)
			m_pVoxRenderer->ResetModel();
	}
}

unsigned int VoxAnimator::GetCurrentAnimation()
{
	return m_CurrentAnimationIndex;
}

void VoxAnimator::ResetAnimationTimer()
{
	m_fFrameTimer = 0.f;
}

void VoxAnimator::SetAnimationFiles(std::vector<std::string>& animationFiles)
{
	unsigned currentAnimationIndex = GetCurrentAnimation();

	ClearAnimationFiles();

	for (size_t animationFileIt = 0; animationFileIt != animationFiles.size(); ++animationFileIt)
		AddAnimationFile(animationFiles[animationFileIt], static_cast<uint32_t>(animationFileIt));

	SetCurrentAnimation(currentAnimationIndex);
}

std::vector<std::string>& VoxAnimator::GetAnimationFiles()
{
	return m_AnimationFiles;
}

void VoxAnimator::SetAnimationFrame(unsigned int newAnimationFrame)
{
	if (GetActiveAnimationModel() == nullptr)
		return;

	if (newAnimationFrame >= GetActiveAnimationModel()->GetFrameCount())
		return;

	unsigned int currentAnimationIndex = GetCurrentAnimation();

	const VoxFrame* newFoxFrame = m_AnimationModels[currentAnimationIndex]->GetFrame(newAnimationFrame);
	if (newFoxFrame != nullptr && m_pVoxRenderer != nullptr)
	{
		m_pVoxRenderer->SetFrame(newFoxFrame);
		
		// Play sound on-frame
		if (m_pAudioPlaylist)
		{
			m_pAudioPlaylist->Play(newAnimationFrame);
		}
	}
}

VoxModel * VoxAnimator::GetActiveAnimationModel()
{
	if (m_AnimationModels.size() != 0 && m_CurrentAnimationIndex < m_AnimationModels.size())
	{
		return m_AnimationModels[m_CurrentAnimationIndex];
	}

	return nullptr;
}

void VoxAnimator::AddAnimationFile(std::string animationFile, unsigned int animationFileIndex)
{
	if (animationFileIndex >= 0 && animationFileIndex <= m_AnimationFiles.size())
	{
		if (animationFileIndex == m_AnimationFiles.size())
		{
			m_AnimationFiles.push_back("");
			m_AnimationModels.push_back(nullptr);
		}
		else
		{
			if (!m_AnimationFiles[animationFileIndex].empty())
				RemoveAnimationFile(animationFileIndex);
		}

		if (!animationFile.empty())
		{
			if (animationFileIndex == m_AnimationFiles.size())
			{
				m_AnimationFiles.push_back("");
				m_AnimationModels.push_back(nullptr);
			}

			m_AnimationModels[animationFileIndex] = GetWorld()->GetApplication()->GetResourceManager().LoadVox(animationFile);
			m_bHasAudio = m_bHasAudio || m_AnimationModels[animationFileIndex]->HasAudio();
		}

		m_AnimationFiles[animationFileIndex] = animationFile;

		SetCurrentAnimation(GetCurrentAnimation());
	}
}

void VoxAnimator::RemoveAnimationFile(unsigned int animationFileIndex)
{
	if (animationFileIndex >= 0 && animationFileIndex < m_AnimationFiles.size())
	{
		if (m_AnimationModels[animationFileIndex] != nullptr)
			m_AnimationModels[animationFileIndex]->Release();

		if (animationFileIndex == m_AnimationFiles.size() - 1)
		{
			m_AnimationFiles.pop_back();
			m_AnimationModels.pop_back();
		}
		else
		{
			m_AnimationModels[animationFileIndex] = nullptr;
			m_AnimationFiles[animationFileIndex] = "";
		}
	}
}

void VoxAnimator::ClearAnimationFiles()
{
	for (unsigned int animationFileIt = 0; animationFileIt <= m_AnimationFiles.size(); ++animationFileIt)
		RemoveAnimationFile(animationFileIt);

	m_bHasAudio = false;
}

void VoxAnimator::CheckCurrentAnimation()
{
	if (m_CurrentAnimationIndex >= m_AnimationFiles.size())
		m_CurrentAnimationIndex = static_cast<uint32_t>((m_AnimationFiles.size() == 0) ? 0 : m_AnimationFiles.size() - 1);

	if (m_CurrentAnimationIndex < 0)
		m_CurrentAnimationIndex = 0;

	SetToCurrentAnimation(0);
}

void VoxAnimator::SetToCurrentAnimation(unsigned int iResetAnimFrame)
{
	if (m_CurrentAnimationIndex != m_AnimationModels.size())
	{
		if (m_AnimationModels[m_CurrentAnimationIndex] != nullptr)
		{
			m_CurrentAnimationIndex = m_CurrentAnimationIndex;
			SetAnimationFrame(iResetAnimFrame);

			if (!m_pAudioSource)
			{
				m_pAudioSource = GetOwner()->GetComponent<AudioSource>();

				if (!m_pAudioSource)
					m_pAudioSource = GetOwner()->AddComponent<AudioSource>();
			}

			if (!m_pAudioPlaylist)
			{
				m_pAudioPlaylist = GetOwner()->GetComponent<AudioPlaylist>();

				if (!m_pAudioPlaylist)
					m_pAudioPlaylist = GetOwner()->AddComponent<AudioPlaylist>();
			}
			else
			{
				if (m_bHasAudio)
					m_pAudioPlaylist->ClearAudioFiles();

				if (m_AnimationModels[m_CurrentAnimationIndex]->HasAudio())
				{
					for (uint32_t i = 0; i < m_AnimationModels[m_CurrentAnimationIndex]->GetFrameCount(); ++i)
					{
						m_pAudioPlaylist->AddAudioFile(m_AnimationModels[m_CurrentAnimationIndex]->GetFrame(i)->GetAudioAsset(), i);
					}
				}
			}
		}
	}
}
