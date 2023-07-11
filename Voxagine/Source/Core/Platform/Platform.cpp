#include "pch.h"
#include "Platform.h"

#include "Core/Application.h"
#include "Core/Settings.h"

#include "Core/Platform/Input/Temp/InputContextNew.h"

//Windows specific headers
#ifdef _WINDOWS
#include "Core/Platform/Time/Windows/WINGameTimer.h"

#include "Core/Platform/Rendering/Objects/Shader.h"

#include "Rendering/DX12/DX12RenderContext.h"
#include "Editor/imgui/Contexts/DXImContext.h"

#include "Window/Windows/WINWindowContext.h"

#include "Editor/imgui/Platforms/W32ImPlatform.h"
#include "Editor/imgui/Contexts/GLImContext.h"

//#include "Rendering/GL/WGLRenderContext.h"
#include "Rendering/GL/EGLRenderContext.h"
#endif

#ifdef _ORBIS
#include "Core/Platform/Time/ORBIS/ORBGameTimer.h"

#include "Core/Platform/Input/ORBIS/ORBInputContext.h"
#include "Rendering/ORBIS/ORBRenderContext.h"
#include "Window/ORBIS/ORBWindowContext.h"
#endif


#include "Audio/FMODContext.h"

#ifdef _WINDOWS
// File Path
#include <processenv.h>
#endif

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

	/* Setup platform */
	switch (platform) {
	case PT_WINDOWS:
	{
#ifdef _WINDOWS
		m_pGameTimer = new WINGameTimer();
		m_pGameTimer->SetFrameLimitSeconds(m_pApplication->GetSettings().GetFrameLimit());
		m_pFixedGameTimer = new WINGameTimer();
		m_pFixedGameTimer->SetFixedTimeStep(true);
		m_pFixedGameTimer->SetTargetElapsedSeconds(m_pApplication->GetSettings().GetFixedTimeStep());

		m_pInputContext = new InputContextNew();

		WINWindowContext* pContext = new WINWindowContext(this);
		m_pWindowContext = pContext;

		pImPlatform = new W32ImPlatform(pContext);

		/* Get application base path */
		char buf[256];
		GetCurrentDirectoryA(256, buf);

		m_BasePath = buf;

		while (std::size_t pos = m_BasePath.find("\\"))
		{
			if (pos == std::string::npos) break;
			m_BasePath.replace(pos, 1, "/");
		}

		m_BasePath += "/";
#else
		assert(false);
#endif
		break;
	}
	case PT_ORBIS:
	{
#ifdef _ORBIS
		m_pGameTimer = new ORBGameTimer();
		m_pGameTimer->SetFrameLimitSeconds(m_pApplication->GetSettings().GetFrameLimit());
		m_pFixedGameTimer = new ORBGameTimer();
		m_pFixedGameTimer->SetFixedTimeStep(true);
		m_pFixedGameTimer->SetTargetElapsedSeconds(m_pApplication->GetSettings().GetFixedTimeStep());

		ORBWindowContext* pContext = new ORBWindowContext(this);
		m_pWindowContext = pContext;

		m_pInputContext = new InputContextNew();
#else
		assert(false);
#endif
		break;
	}
	default:
		assert(false);
	}

	/* Setup rendering API */
	switch (renderingApi) {
	case RA_DIRECTX12:
	{
#ifdef _WINDOWS
		DX12RenderContext* pRenderContext = new DX12RenderContext(this);
		m_pRenderContext = pRenderContext;

		m_pWindowContext->Initialize();
		m_pRenderContext->Initialize();
		m_pInputContext->Initialize(m_pWindowContext);

		pImContext = new DXImContext(pRenderContext);
#else
		assert(false);
#endif
		break;
	}
	case RA_ORBIS:
	{
#ifdef _ORBIS
		ORBRenderContext* pRenderContext = new ORBRenderContext(this);
		m_pRenderContext = pRenderContext;

		m_pRenderContext->Initialize();
		m_pWindowContext->Initialize();
#else
		assert(false);
#endif
		break;
	}
	case RA_OPENGLES:
	case RA_OPENGL:
	{
#ifdef _WINDOWS
		GLRenderContext* pRenderContext;

#ifdef _UNDEFINED_
		if (platform == WINDOWS && api == OPENGL)
			pRenderContext = new WGLRenderContext(this);
		else
#endif
		pRenderContext = new EGLRenderContext(this);

		m_pRenderContext = pRenderContext;

		m_pWindowContext->Initialize();
		m_pInputContext->Initialize(m_pWindowContext);
		m_pRenderContext->Initialize();

		pImContext = new GLImContext(pRenderContext);
#else
		assert(false);
#endif
		break;
	}

	default:
		assert(false);
	}

	/* Setup rendering API */
	switch (audioApi) {
	case AA_FMOD:
		m_pAudioContext = new FMODContext(this);
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
