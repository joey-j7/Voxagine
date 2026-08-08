#include "pch.h"
#include "SDLImPlatform.h"

#include "Core/Platform/Window/SDL/SDLWindowContext.h"
#include "External/imgui/imgui.h"

#include <SDL3/SDL.h>

SDLImPlatform::SDLImPlatform(SDLWindowContext* pWindow) : m_pWindow(pWindow)
{
}

void SDLImPlatform::Initialize()
{
	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = "Voxagine SDL3";
}

void SDLImPlatform::NewFrame()
{
	if (m_pWindow == nullptr)
		return;

	ImGuiIO& io = ImGui::GetIO();

	const UVector2 size = m_pWindow->GetSize();
	io.DisplaySize = ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));

	float fX = 0.f;
	float fY = 0.f;
	const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&fX, &fY);

	io.MousePos = ImVec2(fX, fY);
	io.MouseDown[0] = (buttons & SDL_BUTTON_LMASK) != 0;
	io.MouseDown[1] = (buttons & SDL_BUTTON_RMASK) != 0;
	io.MouseDown[2] = (buttons & SDL_BUTTON_MMASK) != 0;
}
