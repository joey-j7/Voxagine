#pragma once

#include <functional>
#include "External/imgui/imgui.h"

class TextureReference;
class Application;

class EditorButton
{
public:
	friend class Editor;

	EditorButton::EditorButton(
		Application* pContext,
		const std::string& defaultButton,
		const std::string& hoveredButton,
		const std::string& clickedButton,
		const ImVec2& size
	);

	~EditorButton();

	void OnClick(std::function<void(void)> function);

private:
	bool m_bIsHovered = false;
	bool m_bIsClicked = false;
	bool m_bIsUp = false;

	TextureReference* m_pCurrentReference = nullptr;

	TextureReference* m_pDefault = nullptr;
	TextureReference* m_pHovered = nullptr;
	TextureReference* m_pClicked = nullptr;

	ImVec2 m_ButtonSize = ImVec2(0, 0);
};