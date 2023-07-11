#include "pch.h"
#include "ConfigurationWindow.h"

#include "Editor/Editor.h"
#include "Core/Application.h"
#include "Core/Platform/Rendering/RenderContext.h"


ConfigurationWindow::ConfigurationWindow()
{
	SetWindowType(EWT_POPUP);
	SetWindowFlag(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);
}

ConfigurationWindow::~ConfigurationWindow()
{
}

void ConfigurationWindow::Initialize(Editor * pTargetEditor)
{
	Window::Initialize(pTargetEditor);

	ResizeWindow();
}

void ConfigurationWindow::UnInitialize()
{
}

void ConfigurationWindow::ResizeWindow()
{
	RenderContext* pRenderContext = GetEditor()->GetApplication()->GetPlatform().GetRenderContext();
	const UVector2 renderRes = pRenderContext->GetRenderResolution();

	float offsetX = static_cast<float>(renderRes.x) / 10 * 2;
	float offsetY = static_cast<float>(renderRes.y) / 10 * 2;

	SetSize(ImVec2(renderRes.x - (offsetX *2), renderRes.y - (offsetY * 2)));
	SetPosition(ImVec2(offsetX, offsetY));
}

void ConfigurationWindow::CloseWindow()
{
	OnWindowClose();
}
