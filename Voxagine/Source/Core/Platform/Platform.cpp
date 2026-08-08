#include "pch.h"
#include "Platform.h"

#include "Core/Application.h"
#include "Core/Settings.h"

#include "Core/Platform/Input/Temp/InputContextNew.h"

#include "Core/Platform/Time/Chrono/ChronoGameTimer.h"

#include "Core/Platform/Rendering/Objects/Shader.h"

#include "Rendering/Vulkan/VKRenderContext.h"
#include "Window/SDL/SDLWindowContext.h"

#include "Editor/imgui/Contexts/VKImContext.h"
#include "Editor/imgui/Platforms/SDLImPlatform.h"

#ifdef VOXAGINE_FMOD
#include "Audio/FMODContext.h"
#else
#include "Audio/NullAudioContext.h"
#endif

#include <filesystem>

void Platform::Initialize()
{
	Settings& settings = m_pApplication->GetSettings();
	RenderingAPI renderingApi = settings.GetRenderAPIType();
	AudioAPI audioApi = settings.GetAudioAPIType();
	PlatformType platform = settings.GetPlatformType();

	ImPlatform* pImPlatform = nullptr;
	ImContext* pImContext = nullptr;

	m_pGameTimer = nullptr;
	m_pFixedGameTimer = nullptr;

	/* Setup platform. SDL abstracts the window and input, so every desktop
	   platform takes the same path; only the timer and paths differ. */
	switch (platform) {
	case PT_LINUX:
	case PT_WINDOWS:
	{
		m_pGameTimer = new ChronoGameTimer();
		m_pGameTimer->SetFrameLimitSeconds(m_pApplication->GetSettings().GetFrameLimit());
		m_pFixedGameTimer = new ChronoGameTimer();
		m_pFixedGameTimer->SetFixedTimeStep(true);
		m_pFixedGameTimer->SetTargetElapsedSeconds(m_pApplication->GetSettings().GetFixedTimeStep());

		m_pInputContext = new InputContextNew();

		SDLWindowContext* pContext = new SDLWindowContext(this);
		m_pWindowContext = pContext;

		pImPlatform = new SDLImPlatform(pContext);

		/* Get application base path */
		m_BasePath = std::filesystem::current_path().generic_string() + "/";
		break;
	}
	default:
		assert(false);
	}

	/* Setup rendering API */
	switch (renderingApi) {
	case RA_VULKAN:
	{
		VKRenderContext* pRenderContext = new VKRenderContext(this);
		m_pRenderContext = pRenderContext;

		m_pWindowContext->Initialize();
		m_pRenderContext->Initialize();
		m_pInputContext->Initialize(m_pWindowContext);

		pImContext = new VKImContext(pRenderContext);
		break;
	}

	default:
		assert(false);
	}

	/* Setup rendering API */
	switch (audioApi) {
	case AA_FMOD:
#ifdef VOXAGINE_FMOD
		m_pAudioContext = new FMODContext(this);
#else
		/* FMOD needs a proprietary SDK; run silent rather than not at all. */
		m_pAudioContext = new NullAudioContext(this);
#endif
		m_pAudioContext->Initialize();
		break;
	default:
		assert(false);
		break;
	}

	/* Setup imGui */
	m_ImguiSystem.SetContext(pImContext);
	m_ImguiSystem.Initialize(m_pRenderContext);
	m_ImguiSystem.SetPlatform(pImPlatform);
}

void Platform::Deinitialize()
{
	m_pInputContext->Uninitialize();
	m_ImguiSystem.Deinitialize();
	m_pRenderContext->Deinitialize();

	delete m_pInputContext;
	delete m_pWindowContext;
	delete m_pRenderContext;

	delete m_pAudioContext;

	delete m_pGameTimer;
	delete m_pFixedGameTimer;

#ifdef _DEBUG
	RenderContext::Report();
#endif
}
