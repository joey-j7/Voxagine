#include "pch.h"
#include "imgui_dropdown.h"

// #include <External/imgui/imgui_internal.h>

bool ImGui::BeginButtonDropDown(const char* label, const char* selectedItem)
{
	// ImGuiContext& g = *GImGui;
	// const ImGuiStyle& style = g.Style;

	return BeginCombo(label, selectedItem); // The second parameter is the label previewed before opening the combo.
}

void ImGui::EndButtonDropDown()
{
	EndCombo();
}

