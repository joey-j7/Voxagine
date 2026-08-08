#pragma once

#include <string>
#include "Core/Math.h"

#include "Core/Event.h"

#include <rttr/type>
#include <rttr/registration_friend> 

enum RenderingAPI
{
	RA_VULKAN
};

enum AudioAPI
{
	AA_FMOD,
	AA_OPENAL
};

enum PlatformType
{
	PT_LINUX,
	PT_WINDOWS,
	PT_SWITCH,
	PT_ANDROID
};

class Settings
{
public:
	const std::string& GetTitle() const { return m_Title; }
	const std::wstring GetFontPath() const { return std::wstring(m_FontPath.begin(), m_FontPath.end()); }
	const std::string& GetEngineAssetsPath() const { return m_EngineAssetsPath; }

	const std::wstring& GetGPUName() const { return m_GPUName; }
	void SetGPUName(const wchar_t* pGPUName) {
		m_GPUName = std::wstring(pGPUName);
	}

	PlatformType GetPlatformType() const { return m_PlatformType; }
	RenderingAPI GetRenderAPIType() const { return m_RenderApiType; }
	AudioAPI GetAudioAPIType() const { return m_AudioApiType; }

	bool IsVSyncEnabled() const { return m_bEnableVSync; }
	void SetVSync(bool bVSync) { m_bEnableVSync = bVSync; };

	bool IsTearingEnabled() const { return m_bEnableTearing; }
	void SetTearing(bool bTearing) { m_bEnableTearing = bTearing; };

	bool IsFXAAEnabled() const { return m_bFXAAEnabled; }
	void SetFXAA(bool bFXAA) { m_bFXAAEnabled = bFXAA; };

	bool IsShadowEnabled() const { return m_bShadowsEnabled; }
	void SetShadowEnabled(bool bEnabled) { m_bShadowsEnabled = bEnabled; }

	float GetResolutionScale() const { return m_fResolutionScale; }
	void SetResolutionScale(float fResolutionScale) { m_fResolutionScale = std::max(0.1f, fResolutionScale); }

	/* The aspect ratio the game renders at regardless of window shape; the
	   result is letterboxed on present. Zero renders at the window's own
	   ratio, which is what a window manager that ignores size hints will
	   otherwise hand us. */
	float GetLockedAspectRatio() const { return m_fLockedAspectRatio; }
	void SetLockedAspectRatio(float fAspectRatio) { m_fLockedAspectRatio = std::max(0.f, fAspectRatio); }

	void SetFullscreen(bool bFullscreen);
	bool IsFullscreen() const { return m_bFullscreen; }

	const std::string& GetWorldFileExtension() const { return m_WorldFileExtension; }
	const std::string& GetPrefabFileExtension() const { return m_PrebabFileExtension; }

	double GetFixedTimeStep() const { return m_dFixedTimeStep; }
	double GetFrameLimit() const { return m_dFrameLimit; }

	UVector2 m_v2InitialWindowSize = UVector2(1280, 720);

	/* Event list */
	Event<bool> FullscreenChanged;

private:
	/* Determines the window title */
#ifdef EDITOR
	std::string m_Title = "Voxagine";
#else
	std::string m_Title = "Bit Buster";
#endif
	std::wstring m_GPUName = L"Unknown";

	/* Set default paths */
	std::string m_WorldFileExtension = "wld";
	std::string m_PrebabFileExtension = "prefab";
	std::string m_EngineAssetsPath = "Engine/Assets";
	std::string m_FontPath = "Engine/Assets/Fonts/PressStart.spritefont";

	/* _WIN32, not _WINDOWS: the latter came from the old .vcxproj and nothing
	   defines it under CMake. Every Windows compiler defines _WIN32. */
#ifdef _WIN32
	PlatformType m_PlatformType = PT_WINDOWS;
#else
	PlatformType m_PlatformType = PT_LINUX;
#endif
	RenderingAPI m_RenderApiType = RA_VULKAN;

	AudioAPI m_AudioApiType = AA_FMOD;

	double m_dFixedTimeStep = 1.0 / 60.0;
	double m_dFrameLimit = 1.0 / 200.0;

	bool m_bEnableVSync = false;
	bool m_bEnableTearing = !m_bEnableVSync;

	bool m_bFXAAEnabled = true;

	bool m_bShadowsEnabled = true;
	float m_fResolutionScale = 1.f;

	float m_fLockedAspectRatio = 16.f / 9.f;

	bool m_bFullscreen = false;

	RTTR_ENABLE();
	RTTR_REGISTRATION_FRIEND
};