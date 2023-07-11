#include "pch.h"
#include "BaseSettings.h"

#include "Core/JsonSerializer.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<BaseSettings>("BaseSettings")
		.constructor<>()(rttr::policy::ctor::as_object);
}

BaseSettings::BaseSettings()
{
}

BaseSettings::~BaseSettings()
{
}

void BaseSettings::Initialize(JsonSerializer * pSerializer, const std::string & filePath)
{
	m_pSerializer = pSerializer;
	m_FilePath = filePath;

	LoadSettings();
}

void BaseSettings::LoadSettings()
{
	if (IsFilePathValid())
	{
		if (!m_pSerializer->FromJsonFile(*this, m_FilePath))
		{
			InitializeDefaultSettings();
			m_pSerializer->ToJsonFile(*this, m_FilePath, true);
		}
	}

	ResetDirty();
}

void BaseSettings::LoadSettings(const std::string & filePath)
{
	m_FilePath = filePath;
	LoadSettings();
}

void BaseSettings::SaveSettings()
{
	if (IsFilePathValid())
	{
		m_pSerializer->ToJsonFile(*this, m_FilePath, true);
	}

	ResetDirty();
}

void BaseSettings::SaveSettings(const std::string & filePath)
{
	m_FilePath = filePath;
	SaveSettings();
}

void BaseSettings::ResetSettings()
{
	LoadSettings();
}

void BaseSettings::ResetToDefaultSettings()
{
	std::string TempFilePath = m_FilePath;
	InitializeDefaultSettings();
	m_FilePath = TempFilePath;
	SetDirty();
}

bool BaseSettings::HasSettingChangedWithoutSave()
{
	return m_bSettingsDirty;
}

std::string BaseSettings::GetFilePath() const
{
	return m_FilePath;
}

bool BaseSettings::IsFilePathValid() const
{
	return (!GetFilePath().empty());
}

void BaseSettings::SetDirty()
{
	m_bSettingsDirty = true;
}

void BaseSettings::ResetDirty()
{
	m_bSettingsDirty = false;
}
