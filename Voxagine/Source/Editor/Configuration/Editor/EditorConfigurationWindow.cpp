#include "pch.h"
#include "EditorConfigurationWindow.h"

#include "Editor/Editor.h"
#include "Editor/Configuration/Editor/UserSettings.h"

#include "Editor/PropertyRenderer/PropertyRenderer.h"

EditorConfigurationWindow::EditorConfigurationWindow()
{
	SetName("User Settings");
}

EditorConfigurationWindow::~EditorConfigurationWindow()
{
}

void EditorConfigurationWindow::Initialize(Editor * pTargetEditor)
{
	ConfigurationWindow::Initialize(pTargetEditor);

	m_pUserSettings = &GetEditor()->GetUserSettings();
}

void EditorConfigurationWindow::UnInitialize()
{
	ConfigurationWindow::UnInitialize();
}

void EditorConfigurationWindow::OnRender(float fDeltaTime)
{
	ResizeWindow();

	if (m_pUserSettings != nullptr)
	{
		PropertyRenderer& pPropertyRenderer = GetEditor()->GetPropertyRenderer();

		rttr::instance UserSettingsInstance = *m_pUserSettings;
		rttr::type UserSettingsType = UserSettingsInstance.get_type();

		for (rttr::property UserSettingsPropIt : UserSettingsType.get_properties())
			pPropertyRenderer.Render(UserSettingsInstance, UserSettingsPropIt, "###" + UserSettingsType.get_name().to_string() + UserSettingsPropIt.get_name().to_string());
	}

	if (ImGui::Button("Save Settings!"))
	{
		m_pUserSettings->SaveSettings();
	}

	ImGui::SameLine();

	if (ImGui::Button("Restore default Settings!"))
	{
		m_pUserSettings->ResetToDefaultSettings();
	}

	ImGui::SameLine();

	if (ImGui::Button("Close!"))
	{
		m_pUserSettings->ResetSettings();

		ImGui::CloseCurrentPopup();
		CloseWindow();
	}
}