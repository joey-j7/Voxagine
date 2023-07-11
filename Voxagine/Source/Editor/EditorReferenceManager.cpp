#include "pch.h"
#include "EditorReferenceManager.h"

#include <algorithm>

#include "Editor/Editor.h"
#include "Core/Application.h"
#include "Core/LoggingSystem/LoggingSystem.h"
#include <vector>

EditorReferenceManager::EditorReferenceManager()
{
}

EditorReferenceManager::~EditorReferenceManager()
{
}

void EditorReferenceManager::Initialize(Editor * pEditor)
{
	m_pEditor = pEditor;

	// Register how to load Vox Models (.vox)
	RegisterResourceInformation(".vox", [this](std::string filePath)
	{
		return m_pEditor->GetApplication()->GetResourceManager().LoadVox(filePath);
	}
	);

	// Register how to load anim vox models (.anim.vox)
	RegisterResourceInformation(".anim.vox", [this](std::string filePath)
	{
		return m_pEditor->GetApplication()->GetResourceManager().LoadVox(filePath);
	}
	);

	// Register how to load ogg Sound tracks (.ogg)
	RegisterResourceInformation(".ogg", [this](std::string filePath)
	{
		return m_pEditor->GetApplication()->GetResourceManager().LoadSound(filePath);
	}
	);

	// Register how to load png textures (.png)
	RegisterResourceInformation(".png", [this](std::string filePath)
	{
		return m_pEditor->GetApplication()->GetResourceManager().LoadTexture(filePath);
	}
	);
}

void EditorReferenceManager::UnInitialize()
{
	for (auto&& restypeit : m_ResourceReferences)
	{
		for (ReferenceObject* resit : restypeit.second)
		{
			resit->Release();
		}
	}
}

void EditorReferenceManager::ImportFile(std::string filePath)
{
	return ImportFiles({ filePath });
}

void EditorReferenceManager::ImportFiles(const std::vector<std::string>& filePathBatch)
{
	for (const std::string& fileit : filePathBatch)
	{
		std::string FileExtension = FileGetExtension(fileit);
		const ResourceInformation* FileResourceInfo = GetResourceInformation(FileExtension);

		if (FileResourceInfo != nullptr && !HasFile(fileit))
		{
			ReferenceObject* RefResource = FileResourceInfo->LoadFunction(fileit);
			if (RefResource != nullptr && RefResource->IsLoaded())
			{
				FileResourceInfo->ResourceList->push_back(RefResource);
			}
			else
			{
				delete RefResource;
			}
		}
	}
}

bool EditorReferenceManager::HasFile(std::string filePath) const
{
	const ResourceInformation* FileResourceInformation = GetResourceInformation(FileGetExtension(filePath));

	if (FileResourceInformation == nullptr)
	{
		m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "EDITOR", "Editor Reference Manager, has file check upon unsupported resource! ( " + filePath + " )");
		return false;
	}
	else
	{
		std::vector<ReferenceObject*>& References = *FileResourceInformation->ResourceList;
		for (std::vector<ReferenceObject*>::const_iterator refit = References.begin(); refit != References.end(); ++refit)
		{
			if ((*refit)->GetRefPath() == filePath)
				return true;
		}
	}

	return false;
}

void EditorReferenceManager::RegisterResourceInformation(std::string fileExtension, std::function<ReferenceObject*(std::string)> loadFunction)
{
	if (GetResourceInformation(fileExtension) == nullptr)
	{
		m_ResourceReferences[fileExtension] = std::vector<ReferenceObject*>();

		m_ResourceLookUp[fileExtension] = { &m_ResourceReferences[fileExtension] , loadFunction };
	}
	else
	{
		m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "EDITOR", "Editor Reference Manager, register resource information already exists");
	}
}

std::string EditorReferenceManager::FileGetExtension(std::string filePath) const
{
	size_t found = filePath.find(".");

	if (found >= filePath.length())
		m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "EDITOR", "Editor Reference Manager, can't gain file extension of filepath");

	return filePath.substr(filePath.find("."));
}

const EditorReferenceManager::ResourceInformation * EditorReferenceManager::GetResourceInformation(std::string filePath) const
{
	std::unordered_map<std::string, ResourceInformation>::const_iterator found = m_ResourceLookUp.find(filePath);

	return (found != m_ResourceLookUp.end() ? &(found->second) : nullptr);
}
