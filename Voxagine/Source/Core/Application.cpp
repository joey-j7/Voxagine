#include "pch.h"
#include "Application.h"

#ifdef _ORBIS
#include "Core/System/ORBIS/ORBFileSystem.h"
#else
#include "Core/System/Windows/WINFileSystem.h"
#endif

#include "Core/System/FileSystem.h"
#include "Platform/Window/WindowContext.h"
#include "Platform/Input/Temp/InputContextNew.h"
#include "Platform/Rendering/RenderContext.h"
#include "Editor/imgui/ImguiSystem.h"

#include "Core/ECS/World.h"
#include "ECS/Entities/Camera.h"

#include <iostream>
#include "GameTimer.h"
#include "ECS/WorldManager.h"
#include "ECS/Systems/Physics/PhysicsSystem.h"

#include "Core/Platform/Audio/AudioContext.h"
#include "External/optick/optick.h"

Application::Application() :
	m_WorldManager(this),
	m_Platform(this),
	m_ResourceManager(this),
	m_Serializer(m_Settings, m_LoggingSystem)
{
	m_bExit = false;
}

Application::~Application()
{

}

void Application::Run()
{
#ifdef _ORBIS
	m_pFileSystem = new ORBFileSystem(this);
	m_pFileSystem->Initialize();
#else
	m_pFileSystem = new WINFileSystem(this);
	m_pFileSystem->Initialize();
#endif

	m_LoggingSystem.Initialize(this);
	m_Serializer.Initialize(m_pFileSystem);

	LoadSettings();

	m_JobManager.Initialize();
	m_Platform.Initialize();

	OnCreate();

#ifdef EDITOR
	m_Editor.Initialize(this);
#endif

	m_Platform.m_pGameTimer->ResetElapsedTime();
	m_Platform.m_pFixedGameTimer->ResetElapsedTime();

	while (!m_bExit)
	{
		m_Platform.m_pGameTimer->Update([this]()
		{
			OPTICK_FRAME("MainThread");
			float fElapsed = static_cast<float>(m_Platform.m_pGameTimer->GetElapsedSeconds());

			if (m_WorldManager.RequiresSwap())
			{
				m_Platform.GetRenderContext()->WaitForGPU();
				m_WorldManager.SwapWorlds();
			}

			m_Platform.GetWindowContext()->Poll();

			m_Platform.GetInputContext()->Update();
			m_Platform.GetImguiSystem().Update();

			m_Platform.GetRenderContext()->Clear();

			OnUpdate();

			m_JobManager.ProcessFinishedJobs();

			/* Update loop for world */
			World* activeWorld = m_WorldManager.GetTopWorld();

#ifdef EDITOR
			if (activeWorld)
			{
				m_Editor.WorldPreTick(activeWorld);
				m_Editor.WorldTick(activeWorld, fElapsed);
			}

			bool bFixedStep = false;

			m_Platform.m_pFixedGameTimer->Update([&]
			{
				bFixedStep = true;

				RenderContext* pRenderContext = m_Platform.GetRenderContext();

				if (pRenderContext->GetMissedFrames() > 0)
					pRenderContext->ForceUpdate();

				if (activeWorld)
				{
					m_Editor.WorldFixedTick(activeWorld, GetFixedTimer());
					m_Editor.WorldPostFixedTick(activeWorld, GetFixedTimer());
				}
			});

			if (activeWorld)
				m_Editor.WorldPostTick(activeWorld, fElapsed);

			Camera* pCamera = nullptr;
			if (activeWorld) {
				pCamera = activeWorld->GetMainCamera();
			}

			m_Platform.GetRenderContext()->SetCameraData(
				CameraRenderData(
					pCamera->GetMVP(),
					pCamera->GetMV(),
					pCamera->GetView(),
					pCamera->GetProjection(),

					pCamera->GetProjectionValue(),
					pCamera->GetAspectRatio(),
					pCamera->IsOrthographic(),
					pCamera->IsUpdated(),

					Vector4(pCamera->GetTransform()->GetPosition(), 1.0f),
					Vector4(pCamera->GetCameraOffset(), 1.0f)
				)
			);

			pCamera->SetRecalculated(false);

			if (activeWorld)
			{
				if (bFixedStep)
					m_Editor.WorldRender(activeWorld, GetFixedTimer());

				activeWorld->OnDrawGizmos(fElapsed);
			}

			m_Editor.Render(fElapsed);
#else
			if (activeWorld)
			{
				activeWorld->PreTick();
				activeWorld->Tick(fElapsed);
			}

			bool bFixedStep = false;

			m_Platform.m_pFixedGameTimer->Update([&, activeWorld]
			{
				bFixedStep = true;

				RenderContext* pRenderContext = m_Platform.GetRenderContext();

				if (pRenderContext->GetMissedFrames() > 0)
					pRenderContext->ForceUpdate();

				if (activeWorld)
				{
					activeWorld->FixedTick(*m_Platform.m_pFixedGameTimer);
					activeWorld->PostFixedTick(*m_Platform.m_pFixedGameTimer);
				}
			});

			if (activeWorld)
				activeWorld->PostTick(fElapsed);

			Camera* pCamera = nullptr;
			if (activeWorld) {
				pCamera = activeWorld->GetMainCamera();
			}

			m_Platform.GetRenderContext()->SetCameraData(
				CameraRenderData(
					pCamera->GetMVP(),
					pCamera->GetMV(),
					pCamera->GetView(),
					pCamera->GetProjection(),

					pCamera->GetProjectionValue(),
					pCamera->GetAspectRatio(),
					pCamera->IsOrthographic(),
					pCamera->IsUpdated(),

					Vector4(pCamera->GetTransform()->GetPosition(), 1.0f),
					Vector4(pCamera->GetCameraOffset(), 1.0f)
				)
			);

			pCamera->SetRecalculated(false);

			if (activeWorld)
			{
				if (bFixedStep)
				{
					activeWorld->Render(*m_Platform.m_pFixedGameTimer);
				}

				activeWorld->OnDrawGizmos(fElapsed);
			}
#endif

			OnDraw();

			m_Platform.GetRenderContext()->Present();

#ifdef _ORBIS
			uint32_t uiFPS = GetTimer().GetFramesPerSecond();
			const char* pFPS = (std::to_string(uiFPS) + "FPS\n").c_str();
			std::printf(pFPS);
#endif
		});
	}

	OnExit();
	m_JobManager.Deinitialize();
	m_LoggingSystem.UnInitialize();

#ifdef EDITOR
	m_Editor.UnInitialize();
#endif
	
	m_WorldManager.ClearWorlds();
	m_ResourceManager.Unload();
	m_Platform.Deinitialize();

	m_pFileSystem->Deinitialize();
	delete m_pFileSystem;
}

void Application::LoadSettings()
{
	if (!GetSerializer().FromJsonFile(m_Settings, "Settings.vgs"))
	{
		GetSerializer().ToJsonFile(m_Settings, "Settings.vgs", true);
	}
}
