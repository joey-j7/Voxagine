#include "pch.h"

#include <Core/MetaData/PropertyTypeMetaData.h>
#include "Editor/Configuration/Editor/UserSettings.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<UserSettings>("UserSettings")
		.constructor<>()(rttr::policy::ctor::as_object)
		.property("Auto-Save Enabled", &UserSettings::IsAutoSaveEnabled, &UserSettings::EnableAutoSave) (RTTR_PUBLIC)
		.property("Auto-Save Time(in seconds)", &UserSettings::GetAutoSaveTime, &UserSettings::SetAutoSaveTime) (RTTR_PUBLIC);
}

UserSettings::UserSettings()
{
}

UserSettings::~UserSettings()
{
}

void UserSettings::Initialize(JsonSerializer * pSerializer, const std::string & filePath)
{
	BaseSettings::Initialize(pSerializer, filePath);
}

void UserSettings::EnableAutoSave(bool bAutoSaveEnabled)
{
	m_bAutoSaveEnabled = bAutoSaveEnabled;
	SetDirty();
}

bool UserSettings::IsAutoSaveEnabled() const
{
	return m_bAutoSaveEnabled;
}

void UserSettings::SetAutoSaveTime(unsigned int uiAutoSaveTime)
{
	if (uiAutoSaveTime == 0)
		uiAutoSaveTime = 1;

	m_uiAutoSaveTime = uiAutoSaveTime;
	SetDirty();
}

unsigned int UserSettings::GetAutoSaveTime() const
{
	return m_uiAutoSaveTime;
}

void UserSettings::InitializeDefaultSettings()
{
	m_bAutoSaveEnabled = true;
	m_uiAutoSaveTime = 5;
}
