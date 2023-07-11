#include "pch.h"
#include "EntityWizard.h"

#include "Editor/Editor.h"
#include "Core/Application.h"
#include "Core/ECS/WorldManager.h"
#include "Core/ECS/World.h"
#include <Core/Platform/Rendering/RenderContext.h>
#include <Core/Platform/Platform.h>

EntityWizard::EntityWizard()
{
}

EntityWizard::~EntityWizard()
{
}

void EntityWizard::Initialize(Editor * pTargetEditor)
{
	Window::Initialize(pTargetEditor);

	SetName("Entity Wizard");

	UVector2 renderRes = GetEditor()->GetApplication()->GetPlatform().GetRenderContext()->GetRenderResolution();

	SetSize(ImVec2(static_cast<float>(renderRes.x), static_cast<float>(renderRes.y)));
	SetPosition(ImVec2(0, 0));
}

void EntityWizard::UnInitialize()
{
	Window::UnInitialize();
}

void EntityWizard::Tick(float fDeltaTime)
{
	Window::Tick(fDeltaTime);
}

// void EntityWizard::Render(float fDeltaTime)
// {
// 	Window::Render(fDeltaTime);
// }
// 
// void EntityWizard::EndRendering(float fDeltaTime)
// {
// 	Window::EndRendering(fDeltaTime);
// }

void EntityWizard::OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution)
{
	SetSize(ImVec2(static_cast<float>(a_uiWidth), static_cast<float>(a_uiHeight)));
	SetPosition(ImVec2(0, 0));

	ImGui::SetWindowPos(GetName().data(), GetPosition());
	ImGui::SetWindowSize(GetName().data(), GetSize());
}
