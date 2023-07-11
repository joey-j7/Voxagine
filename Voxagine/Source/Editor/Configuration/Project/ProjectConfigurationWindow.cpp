#include "pch.h"
#include "ProjectConfigurationWindow.h"

#include "Editor/Editor.h"
#include "Editor/Configuration/Project/ProjectSettings.h"

ProjectConfigurationWindow::ProjectConfigurationWindow()
{
	SetName("Project Settings");
}

ProjectConfigurationWindow::~ProjectConfigurationWindow()
{
}

void ProjectConfigurationWindow::Initialize(Editor * pTargetEditor)
{
	ConfigurationWindow::Initialize(pTargetEditor);

	m_pProjectSettings = &GetEditor()->GetProjectSettings();
}

void ProjectConfigurationWindow::UnInitialize()
{
	ConfigurationWindow::UnInitialize();
}

void ProjectConfigurationWindow::OnRender(float fDeltaTime)
{
	ResizeWindow();

	ImVec2 ButtonSize = ImVec2(96, 16);

	if (ImGui::Button("Got it!", ButtonSize))
	{
		ImGui::CloseCurrentPopup();
		CloseWindow();
	}
}
