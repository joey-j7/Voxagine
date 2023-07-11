#include "pch.h"
#include "ImguiSystem.h"

#include "External/imgui/imgui.h"

#include "Contexts/ImContext.h"
#include "Platforms/ImPlatform.h"

#include "Core/Application.h"
#include "Core/Settings.h"

#include "Core/ECS/WorldManager.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/World.h"

#include "Core/ECS/Systems/Rendering/RenderSystem.h"

#include "Core/Platform/Input/Temp/InputKeyIdentifiers.h"
#include <Core/Platform/Platform.h>
#include <Core/Platform/Window/WindowContext.h>
#include <External/imguizmo/ImGuizmo.h>
#include "External/optick/optick.h"

void ImguiSystem::Initialize(RenderContext* pRenderContext)
{
#ifndef _ORBIS
	m_pRenderContext = pRenderContext;

	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	io.WantCaptureKeyboard = true;

	// Setup style
	InitStyle();

 	// Bind mappings
	io.KeyMap[ImGuiKey_::ImGuiKey_Tab] = IK_TAB;
	io.KeyMap[ImGuiKey_::ImGuiKey_LeftArrow] = IK_LEFT;
	io.KeyMap[ImGuiKey_::ImGuiKey_RightArrow] = IK_RIGHT;
	io.KeyMap[ImGuiKey_::ImGuiKey_UpArrow] = IK_UP;
	io.KeyMap[ImGuiKey_::ImGuiKey_DownArrow] = IK_DOWN;
	io.KeyMap[ImGuiKey_::ImGuiKey_PageUp] = IK_PAGEUP;
	io.KeyMap[ImGuiKey_::ImGuiKey_PageDown] = IK_PAGEDOWN;
	io.KeyMap[ImGuiKey_::ImGuiKey_Home] = IK_HOME;
	io.KeyMap[ImGuiKey_::ImGuiKey_End] = IK_END;
	io.KeyMap[ImGuiKey_::ImGuiKey_Insert] = IK_INSERT;
	io.KeyMap[ImGuiKey_::ImGuiKey_Delete] = IK_DELETE;
	io.KeyMap[ImGuiKey_::ImGuiKey_Backspace] = IK_BACK;
	io.KeyMap[ImGuiKey_::ImGuiKey_Space] = IK_SPACE;
	io.KeyMap[ImGuiKey_::ImGuiKey_Enter] = IK_ENTER;
	io.KeyMap[ImGuiKey_::ImGuiKey_Escape] = IK_ESCAPE;
	io.KeyMap[ImGuiKey_::ImGuiKey_A] = IK_A;  // for text edit CTRL+A: select all
	io.KeyMap[ImGuiKey_::ImGuiKey_C] = IK_C;  // for text edit CTRL+C: copy
	io.KeyMap[ImGuiKey_::ImGuiKey_V] = IK_V;  // for text edit CTRL+V: paste
	io.KeyMap[ImGuiKey_::ImGuiKey_X] = IK_X;  // for text edit CTRL+X: cut
	io.KeyMap[ImGuiKey_::ImGuiKey_Y] = IK_Y;  // for text edit CTRL+Y: redo
	io.KeyMap[ImGuiKey_::ImGuiKey_Z] = IK_Z;  // for text edit CTRL+Z: undo

	io.ImeWindowHandle = m_pRenderContext->GetPlatform()->GetWindowContext()->GetHandle();
#endif
}

void ImguiSystem::Update()
{
	OPTICK_CATEGORY("Editor render", Optick::Category::Rendering);
	OPTICK_EVENT();
#ifndef _ORBIS
	// Start the Dear ImGui frame
	m_pContext->NewFrame();
	m_pPlatform->NewFrame();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = static_cast<float>(m_pRenderContext->GetRenderResolution().x);
	io.DisplaySize.y = static_cast<float>(m_pRenderContext->GetRenderResolution().y);

	io.MousePos.x *= m_pRenderContext->GetRenderScale();
	io.MousePos.y *= m_pRenderContext->GetRenderScale();

	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

//#if EDITOR == 0
//	ImGui::Begin("DISCLAIMER", 0, ImGuiWindowFlags_AlwaysAutoResize);
//
//	ImGui::Text("This application is strictly private,");
//	ImGui::Text("confidential and personal to its");
//	ImGui::Text("recipients and should not be copied,");
//	ImGui::Text("distributed or reproduced in whole or");
//	ImGui::Text("in part, nor passed to any third party.\n\n");
//
//	ImGui::Text("Controls:");
//	ImGui::Text("WASD       : Move Character");
//	ImGui::Text("Space      : Shoot Projectile");
//	ImGui::Text("Arrow Keys : Rotate Camera");
//	ImGui::Text("Alt + Enter: Toggle Fullscreen\n\n");
//
//	ImGui::Text("Active GPU device:\n%ls\n\n", m_pRenderContext->GetPlatform()->GetApplication()->GetSettings().GetGPUName().c_str());
//
//	ImGui::Text("Current render resolution:\n%d x %d\n\n", m_pRenderContext->GetRenderResolution().x, m_pRenderContext->GetRenderResolution().y);
//
//	PhysicsSystem* pPhysics = m_pRenderContext->GetPlatform()->GetApplication()->GetWorldManager().GetTopWorld()->GetSystem<PhysicsSystem>();
//	ImGui::Text("Simulating particles: %i\n", pPhysics->m_uiActiveParticleCount);
//
//	uint32_t uiCPUTime = m_pRenderContext->GetPlatform()->GetApplication()->GetTimer().GetFramesPerSecond();
//	uint32_t uiGPUTime = m_pRenderContext->GetFPS();
//
//	ImGui::Text("CPU: %.3f ms/frame (%d FPS)", 1000.0f / uiCPUTime, uiCPUTime);
//	ImGui::Text("GPU: %.3f ms/frame (%d FPS)", 1000.0f / uiGPUTime, uiGPUTime);
//
//	ImGui::End();
//#endif
#endif
}

void ImguiSystem::Draw()
{
#ifndef _ORBIS
	ImGui::Render();
	m_pContext->Draw(ImGui::GetDrawData());
#endif
}

void ImguiSystem::Deinitialize()
{
#ifndef _ORBIS
	m_pContext->Deinitialize();

	delete m_pContext;
	delete m_pPlatform;

	ImGui::DestroyContext();
#endif
}

void ImguiSystem::InitStyle()
{
#ifndef _ORBIS
	ImGui::StyleColorsDark();

	ImGuiStyle * style = &ImGui::GetStyle();

	style->WindowRounding = 0.0f;
	style->WindowBorderSize = 4.0f;
	style->FrameRounding = 0.0f;

	style->ScrollbarRounding = 2.0f;
	style->ScrollbarSize = 10.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);

	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.192f, 0.219f, 0.9f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.10f, 0.15f, 0.95f);

	style->Colors[ImGuiCol_Border] = ImVec4(0.145f, 0.156f, 0.180f, 0.9f);

	style->Colors[ImGuiCol_FrameBg] = ImVec4(0.215f, 0.235f, 0.274f, 0.9f);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.215f, 0.235f, 0.274f, 0.9f);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.215f, 0.235f, 0.274f, 0.9f);

	style->Colors[ImGuiCol_Header] = ImVec4(0.103f, 0.501f, 0.737f, 0.9f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.082f, 0.454f, 0.658f, 0.9f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.101f, 0.623f, 0.737f, 0.9f);

	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.103f, 0.501f, 0.737f, 0.9f);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.103f, 0.501f, 0.737f, 0.9f);

	style->Colors[ImGuiCol_Button] = ImVec4(0.103f, 0.501f, 0.737f, 0.9f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.082f, 0.454f, 0.658f, 0.9f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.101f, 0.623f, 0.737f, 0.9f);

	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.145f, 0.156f, 0.180f, 0.9f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.145f, 0.156f, 0.180f, 0.9f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.145f, 0.156f, 0.180f, 0.9f);

	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.3f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
#endif
}