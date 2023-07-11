#pragma once

#include "Editor/Configuration/ConfigurationWindow.h"

class ProjectSettings;

class ProjectConfigurationWindow : public ConfigurationWindow
{
public:
	ProjectConfigurationWindow();
	virtual ~ProjectConfigurationWindow();

	void Initialize(Editor* pTargetEditor) override;
	void UnInitialize() override;

protected:
	void OnRender(float fDeltaTime) override;

private:
	ProjectSettings* m_pProjectSettings = nullptr;
};