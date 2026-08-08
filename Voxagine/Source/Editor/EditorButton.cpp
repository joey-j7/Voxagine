#include "pch.h"
#include "EditorButton.h"

#include "Core/Application.h"
#include "Core/Resources/Formats/TextureReference.h"
#include "External/imgui/imgui.h"
#include "Core/Platform/Rendering/RenderContext.h"

EditorButton::EditorButton(Application* pContext, const std::string& defaultButton, const std::string& hoveredButton, const std::string& clickedButton, const ImVec2& size)
{
	m_pDefault = pContext->GetResourceManager().LoadTexture(defaultButton);
	m_pHovered = pContext->GetResourceManager().LoadTexture(hoveredButton);
	m_pClicked = pContext->GetResourceManager().LoadTexture(clickedButton);

	m_ButtonSize = size;
	m_pCurrentReference = m_pDefault;
}

EditorButton::~EditorButton()
{
	if (m_pDefault)
		m_pDefault->Release();

	if (m_pHovered)
		m_pHovered->Release();

	if (m_pClicked)
		m_pClicked->Release();
}

void EditorButton::OnClick(std::function<void(void)> function)
{
	/* ImGui draws nothing for a null texture, but the hover and click logic
	   below still has to run. */
	View* pTexture = m_pCurrentReference != nullptr ? m_pCurrentReference->TextureView : nullptr;

	if (ImGui::ImageButton(static_cast<ImTextureID>(pTexture), m_ButtonSize, ImVec2(0, 0), ImVec2(1, 1), 0) || m_bIsUp)
	{
		function();
		m_bIsUp = false;
	}

	m_bIsHovered = ImGui::IsItemHovered();
	m_bIsUp = m_bIsClicked && !ImGui::IsMouseDown(0);
	m_bIsClicked = ImGui::IsMouseDown(0) && m_bIsHovered;

	m_pCurrentReference = m_bIsClicked ? m_pClicked : m_bIsHovered ? m_pHovered : m_pDefault;
}
