#include "pch.h"

#include <Core/MetaData/PropertyTypeMetaData.h>
#include "Editor/Configuration/Project/ProjectSettings.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<ProjectSettings>("ProjectSettings")
		.constructor<>()(rttr::policy::ctor::as_object)
		.property("DefaultMap", &ProjectSettings::GetDefaultMap, &ProjectSettings::SetDefaultMap) (RTTR_PUBLIC)
		.property("ContentFolderPath", &ProjectSettings::GetContentFolderPath, &ProjectSettings::SetContentFolderPath) (RTTR_PUBLIC);
}

ProjectSettings::ProjectSettings()
{
}

ProjectSettings::~ProjectSettings()
{
}

void ProjectSettings::Initialize(JsonSerializer * pSerializer, const std::string & filePath)
{
	BaseSettings::Initialize(pSerializer, filePath);
}

void ProjectSettings::SetDefaultMap(std::basic_string<char> Map)
{
	m_DefaultMap = Map;
	SetDirty();
}

std::string ProjectSettings::GetDefaultMap() const
{
	return m_DefaultMap;
}

void ProjectSettings::SetContentFolderPath(std::string Path)
{
	m_ContentFolderPath = Path;
	SetDirty();
}

std::string ProjectSettings::GetContentFolderPath() const
{
	return m_ContentFolderPath;
}

void ProjectSettings::InitializeDefaultSettings()
{
	m_ContentFolderPath = "Content";
}