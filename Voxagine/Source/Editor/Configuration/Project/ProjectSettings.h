#pragma once

#include "Editor/Configuration/BaseSettings.h"

class ProjectSettings : public BaseSettings
{
public:
	ProjectSettings();
	virtual ~ProjectSettings();

	void Initialize(JsonSerializer* pSerializer, const std::string& filePath = std::string()) override;

	void SetDefaultMap(std::string Map);
	std::string GetDefaultMap() const;

	void SetContentFolderPath(std::string Path);
	std::string GetContentFolderPath() const;
private:
	void InitializeDefaultSettings() override;

	std::string m_DefaultMap = "Content/Worlds/Menus/SplashScreen.wld";
	std::string m_ContentFolderPath = "";

	RTTR_ENABLE(BaseSettings)
};