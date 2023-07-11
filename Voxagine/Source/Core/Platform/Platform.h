#pragma once

#include "Editor/imgui/ImguiSystem.h"
#include "PlatformData.h"

#include <string>

class Application;

class InputContextNew;
class RenderContext;
class WindowContext;
class GameTimer;

class AudioContext;

class Platform
{
	friend class Application;

public:
	Platform(Application* pApp): m_pApplication(pApp) {}

	virtual void Initialize();
	virtual void Deinitialize();

	InputContextNew* GetInputContext() const { return m_pInputContext; }
	RenderContext* GetRenderContext() const { return m_pRenderContext; }
	WindowContext* GetWindowContext() const { return m_pWindowContext; }

	PlatformData& GetPlatformData() { return m_PlatformData; };

	ImguiSystem& GetImguiSystem() { return m_ImguiSystem; }
	const GameTimer* GetGameTimer() const { return m_pGameTimer; }
	const GameTimer* GetFixedTimer() const { return m_pFixedGameTimer; }

	const std::string& GetBasePath() const { return m_BasePath; }

	AudioContext* GetAudioContext() const { return m_pAudioContext; }

	Application* GetApplication() { return m_pApplication; }

protected:
	Application* m_pApplication;
	PlatformData m_PlatformData;

	ImguiSystem m_ImguiSystem;

	RenderContext* m_pRenderContext = nullptr;
	WindowContext* m_pWindowContext = nullptr;
	InputContextNew* m_pInputContext = nullptr;

	AudioContext* m_pAudioContext = nullptr;

	GameTimer* m_pGameTimer = nullptr;
	GameTimer* m_pFixedGameTimer = nullptr;

	std::string m_BasePath = "";
};