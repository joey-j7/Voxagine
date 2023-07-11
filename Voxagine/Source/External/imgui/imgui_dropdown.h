#pragma once

#include "imgui.h"

namespace ImGui
{
	IMGUI_API bool BeginButtonDropDown(const char* label, const char* selectedItem);
	IMGUI_API void EndButtonDropDown();
}