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
private:
	virtual void InitializeDefaultSettings() override;

private:
	bool m_bAutoSaveEnabled = false;
	unsigned int m_uiAutoSaveTime = 0;
};