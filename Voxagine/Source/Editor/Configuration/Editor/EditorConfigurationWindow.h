#pragma once

#include "Editor/Configuration/ConfigurationWindow.h"

class UserSettings;

class EditorConfigurationWindow : public ConfigurationWindow
{
public:
	EditorConfigurationWindow();
	virtual ~EditorConfigurationWindow();

	void Initialize(Editor* pTargetEditor) override;
	void UnInitialize() override;

private:
	virtual void OnRender(float fDeltaTime) override;

private:
	UserSettings* m_pUserSettings = nullptr;
};