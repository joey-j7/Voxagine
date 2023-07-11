#pragma once

#include <string>

#include "External/imgui/imgui.h"
#include "Core/Math.h"

enum EditorWindowType
{
	EWT_WINDOW,
	EWT_POPUP,
};

class Editor;
class Window
{
public:
	Window();
	virtual ~Window();
	
	virtual void Initialize(Editor* pTargetEditor);
	virtual void UnInitialize();

	virtual void Tick(float fDeltaTime);
	void Render(float fDeltaTime);

	virtual void OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution) = 0;

	void SetVisibility(bool bNewVisibility);
	bool IsVisible() const;

	void SetEnabled(bool bNewEnabled);
	bool IsEnabled() const;

	void SetPosition(ImVec2 newPosition);
	ImVec2 GetPosition() const;

	void SetSize(ImVec2 newSize);
	ImVec2 GetSize() const;

	std::string GetName() const;

	void AddWindowFlag(ImGuiWindowFlags windowFlagToAdd);
	void RemoveWindowFlag(ImGuiWindowFlags windowFlagToAdd);

	void SetWindowFlag(ImGuiWindowFlags newWindowFlags);
	void ClearWindowFlags();

	ImGuiWindowFlags GetWindowFlags() const;

protected:
	virtual void OnPreRender(float fDeltaTime);
	virtual void OnRender(float fDeltaTime) = 0;
	virtual void OnPostRender(float fDeltaTime);

	void SetName(std::string NewName);
	void SetWindowType(EditorWindowType windowType);

	Editor* GetEditor();

private:
	void RenderWindow(float fDeltaTime);
	void RenderPopUpWindw(float fDeltaTime);

private:
	Editor* m_pEditor = nullptr;
	bool m_bIsImguiWindowInitialized = false;
	EditorWindowType m_WindowType = EditorWindowType::EWT_WINDOW;

	std::string m_Name;
	ImVec2 m_Position;
	ImVec2 m_Size;

	bool m_bVisibility;
	bool m_bEnabled;

	ImGuiWindowFlags m_WindowFlags;
};