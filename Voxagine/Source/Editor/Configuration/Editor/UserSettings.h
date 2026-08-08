#pragma once

#include "Editor/Configuration/BaseSettings.h"

class UserSettings : public BaseSettings
{
RTTR_ENABLE(BaseSettings)
public:
	UserSettings();
	~UserSettings();

	void Initialize(JsonSerializer* pSerializer, const std::string& filePath = std::string()) override;

	void EnableAutoSave(bool bAutoSaveEnabled);
	bool IsAutoSaveEnabled() const;

	void SetAutoSaveTime(unsigned int uiAutoSaveTime);
	unsigned int GetAutoSaveTime() const;

	// World the editor opens on launch, relative to the content folder.
	// Empty falls back to ProjectSettings::GetDefaultMap() - per-user, not
	// checked in, so it doesn't affect the shipped game's boot world.
	void SetStartupWorld(std::string sStartupWorld);
	std::string GetStartupWorld() const;
private:
	virtual void InitializeDefaultSettings() override;

private:
	bool m_bAutoSaveEnabled = false;
	unsigned int m_uiAutoSaveTime = 0;
	std::string m_sStartupWorld;
};