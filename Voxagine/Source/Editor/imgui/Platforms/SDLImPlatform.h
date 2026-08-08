#pragma once

#include "Editor/imgui/Platforms/ImPlatform.h"

class SDLWindowContext;

/* Feeds ImGui its display size and input, replacing W32ImPlatform's WndProc
   hooks with SDL events. */
class SDLImPlatform : public ImPlatform
{
public:
	SDLImPlatform(SDLWindowContext* pWindow);

	virtual void Initialize() override;
	virtual void NewFrame() override;

private:
	SDLWindowContext* m_pWindow = nullptr;
};
