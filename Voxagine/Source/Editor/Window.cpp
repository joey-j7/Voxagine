#include "pch.h"
#include "Window.h"

Window::Window()
	: m_pEditor(nullptr)
	, m_Name(" ")
	, m_Position()
	, m_Size()
	, m_bVisibility(true)
	, m_bEnabled(true)
	, m_WindowFlags(ImGuiWindowFlags_::ImGuiWindowFlags_None)
{
}

Window::~Window()
{
	// Does nothing by default
}

void Window::Initialize(Editor * pTargetEditor)
{
	m_pEditor = pTargetEditor;
}

void Window::UnInitialize()
{
	// Does nothing by default
}

void Window::Tick(float fDeltaTime)
{
	(void)fDeltaTime;
	// Does nothing by default
}

void Window::Render(float fDeltaTime)
{
	(void)fDeltaTime;

	if (!m_bIsImguiWindowInitialized)
	{
		ImGui::SetNextWindowPos(m_Position);
		ImGui::SetNextWindowSize(m_Size);
		m_bIsImguiWindowInitialized = true;
	}

	OnPreRender(fDeltaTime);

	switch (m_WindowType)
	{
	case (EWT_WINDOW):
		RenderWindow(fDeltaTime);
		break;
	case (EWT_POPUP):
		RenderPopUpWindw(fDeltaTime);
		break;
	}

	OnPostRender(fDeltaTime);
}

void Window::SetVisibility(bool bNewVisibility)
{
	m_bVisibility = bNewVisibility;
}

bool Window::IsVisible() const
{
	return m_bVisibility;
}

void Window::SetEnabled(bool bNewEnabled)
{
	m_bEnabled = bNewEnabled;
}

bool Window::IsEnabled() const
{
	return m_bEnabled;
}

void Window::SetPosition(ImVec2 newPosition)
{
	m_Position = newPosition;
}

ImVec2 Window::GetPosition() const
{
	return m_Position;
}

void Window::SetSize(ImVec2 newSize)
{
	m_Size = newSize;
}

ImVec2 Window::GetSize() const
{
	return m_Size;
}

std::string Window::GetName() const
{
	return m_Name;
}

void Window::AddWindowFlag(ImGuiWindowFlags windowFlagToAdd)
{
	ImGuiWindowFlags NewWindowFlags = GetWindowFlags() | windowFlagToAdd;
	SetWindowFlag(NewWindowFlags);
}

void Window::RemoveWindowFlag(ImGuiWindowFlags windowFlagToAdd)
{
	windowFlagToAdd = windowFlagToAdd & GetWindowFlags();

	ImGuiWindowFlags NewWindowFlags = GetWindowFlags() ^ windowFlagToAdd;
	SetWindowFlag(NewWindowFlags);
}

void Window::SetWindowFlag(ImGuiWindowFlags newWindowFlags)
{
	m_WindowFlags = newWindowFlags;
}

void Window::ClearWindowFlags()
{
	m_WindowFlags = ImGuiWindowFlags_None;
}

ImGuiWindowFlags Window::GetWindowFlags() const
{
	return m_WindowFlags;
}

void Window::OnPreRender(float fDeltaTime)
{
}

void Window::OnPostRender(float fDeltaTime)
{
}

void Window::SetName(std::string NewName)
{
	if (!NewName.empty())
		m_Name = NewName;
}

void Window::SetWindowType(EditorWindowType windowType)
{
	m_WindowType = windowType;
}

Editor * Window::GetEditor()
{
	return m_pEditor;
}

void Window::RenderWindow(float fDeltaTime)
{
	ImGui::Begin(GetName().c_str(), NULL, GetWindowFlags());

	OnRender(fDeltaTime);

	ImGui::End();
}

void Window::RenderPopUpWindw(float fDeltaTime)
{
	if (!ImGui::IsPopupOpen((GetName()).c_str()))
		ImGui::OpenPopup((GetName()).c_str());

	ImGui::BeginPopupModal(GetName().c_str(), NULL, GetWindowFlags());

	OnRender(fDeltaTime);

	ImGui::EndPopup();
}
