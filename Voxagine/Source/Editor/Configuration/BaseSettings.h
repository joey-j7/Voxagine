#pragma once

#include <string>

class JsonSerializer;

class BaseSettings
{
public:
	BaseSettings();
	virtual ~BaseSettings();

	virtual void Initialize(JsonSerializer* pSerializer, const std::string& filePath = std::string());

	void LoadSettings();
	void LoadSettings(const std::string& filePath);
	void SaveSettings();
	void SaveSettings(const std::string& filePath);

	void ResetSettings();
	void ResetToDefaultSettings();
	bool HasSettingChangedWithoutSave();

	std::string GetFilePath() const;
	bool IsFilePathValid() const;
protected:
	void SetDirty();
	void ResetDirty();

private:
	virtual void InitializeDefaultSettings() {};
private:
	JsonSerializer* m_pSerializer = nullptr;
	std::string m_FilePath = "";
	bool m_bSettingsDirty = false;

	RTTR_ENABLE()
};