#include "pch.h"
#include "ResourceManager.h"

#include "Core/Application.h"
#include "Core/Platform/Audio/AudioContext.h"
#include "Core/Platform/Rendering/RenderContextInc.h"

ResourceManager::ResourceManager(Application* pApp)
{
	m_pApp = pApp;
}

void ResourceManager::Unload()
{
	m_voxManager.ClearAll();
	m_textureManager.ClearAll();
	m_soundManager.ClearAll();
}

TextureReference* ResourceManager::LoadTexture(const std::string& filePath)
{
	TextureReference* pTextureRef = m_textureManager.AddReference(filePath);

	if (!pTextureRef->IsLoaded())
	{
		pTextureRef->SetContext(m_pApp->GetPlatform().GetRenderContext());
		pTextureRef->SetFileSystem(m_pApp->GetFileSystem());

		pTextureRef->Load(filePath);

		if (!pTextureRef->IsLoaded())
			LogFailedLoadResourceMessage("Texture", filePath);
	}

	return pTextureRef;
}

FMODSoundReference* ResourceManager::LoadSound(const std::string& filePath)
{
	FMODSoundReference* pSoundRef = m_soundManager.AddReference(filePath);
	AudioContext* pAudioContext = m_pApp->GetPlatform().GetAudioContext();

	if (!pSoundRef->IsLoaded() && pAudioContext)
	{
		pSoundRef->SetContext(pAudioContext);
		pSoundRef->SetFileSystem(m_pApp->GetFileSystem());

		pSoundRef->Load(filePath);
		pSoundRef->m_fLength = pAudioContext->GetLength(pSoundRef);

		if (!pSoundRef->IsLoaded())
			LogFailedLoadResourceMessage("Sound", filePath);
	}

	return pSoundRef;
}

VoxModel* ResourceManager::LoadVox(const std::string& filePath)
{
	VoxModel* pModel = m_voxManager.AddReference(filePath);
	if (!pModel->IsLoaded())
	{
		pModel->SetContext(m_pApp->GetPlatform().GetRenderContext()->Get());
		pModel->SetFileSystem(m_pApp->GetFileSystem());

		pModel->Load(filePath);

		if (!pModel->IsLoaded())
			LogFailedLoadResourceMessage("VoxModel", filePath);
	}
	return pModel;
}

std::vector<VoxModel*> ResourceManager::LoadVoxBatch(const std::string& filePath, const std::string& fileName)
{
	std::vector<VoxModel*> models = {};

	for (uint32_t i = 0; i < UINT32_MAX; ++i)
	{
		std::string file = filePath + fileName + "_" + std::to_string(i) + ".vox";
		VoxModel* pModel = m_voxManager.AddReference(file);

		if (!pModel->IsLoaded())
		{
			pModel->SetContext(m_pApp->GetPlatform().GetRenderContext()->Get());
			pModel->SetFileSystem(m_pApp->GetFileSystem());

			if (!pModel->Load(file))
			{
				m_voxManager.RemoveReference(file);
				break;
			}
		}

		models.push_back(pModel);
	}

	return models;
}

VoxModel* ResourceManager::CreateHollowVox(const std::string& filePath)
{
	std::string newFilePath = filePath;
	auto len = filePath.find_last_of('/');

	if (len == std::string::npos)
	{
		len = 0;
	}
	else
	{
		len += 1;
	}

	newFilePath.insert(len, "hollow_");

	VoxModel* pHollowModel = m_voxManager.AddReference(newFilePath);
	if (!pHollowModel->IsLoaded())
	{
		pHollowModel->SetContext(m_pApp->GetPlatform().GetRenderContext()->Get());
		pHollowModel->SetFileSystem(m_pApp->GetFileSystem());

		pHollowModel->Load(filePath);
		
		if (!pHollowModel->IsLoaded())
		{
			return nullptr;
		}

		pHollowModel->MakeHollow(newFilePath);
	}

	return pHollowModel;
}

void ResourceManager::GetResourceFilePaths(const std::string fileExtension, std::vector<std::string>& resourceFilePaths)
{
	if (fileExtension == "vox")
	{
		m_voxManager.GetResourceFilePaths(resourceFilePaths, fileExtension);
		return;
	}

	if (fileExtension == "anim.vox")
	{
		m_voxManager.GetResourceFilePaths(resourceFilePaths, fileExtension);
		return;
	}

	if (fileExtension == "ogg")
	{
		m_soundManager.GetResourceFilePaths(resourceFilePaths, fileExtension);
		return;
	}

	if (fileExtension == "png")
	{
		m_textureManager.GetResourceFilePaths(resourceFilePaths, fileExtension);
		return;
	}

	if(fileExtension == "wld")
	{
		resourceFilePaths = m_pApp->GetWorldManager().GetWorldFiles();
		return;
	}
}

void ResourceManager::LogFailedLoadResourceMessage(const std::string & resourceTypeName, const std::string & filePath)
{
	m_pApp->GetLoggingSystem().Log(LOGLEVEL_WARNING, "ResourceManager", "Failed to load " + resourceTypeName + " resource! Resource doesn't exists or not readable at location: " + filePath);
}
