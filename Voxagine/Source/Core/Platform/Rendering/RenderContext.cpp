#include "pch.h"

#include <algorithm>

#include "External/imgui/imgui.h"
#include "RenderContext.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Settings.h"

#include "Core/Platform/Window/WindowContext.h"

#include "Core/ECS/World.h"
#include "Core/ECS/WorldManager.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Chunk/ChunkSystem.h"
#include "Core/ECS/Systems/Chunk/FarFieldBaker.h"

#include "Core/Platform/Rendering/FrameProfiler.h"
#include "RenderDefines.h"
#include "Core/Platform/Rendering/Managers/TextureManagerInc.h"
#include "Core/Platform/Rendering/CommandEngineInc.h"
#include "Core/Platform/Rendering/RenderContextInc.h"
#include "Core/Platform/Rendering/RenderPassInc.h"

/* Object */
#include "Core/Platform/Rendering/Objects/Shader.h"
#include "Core/Platform/Rendering/Objects/View.h"
#include "Core/Platform/Rendering/Objects/Buffer.h"
#include "Core/Platform/Rendering/Objects/Sampler.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"

/* Passes */
#include "Core/Platform/Rendering/Passes/ParticlePass.h"
#include "Core/Platform/Rendering/Passes/DebugPass.h"
#include "Core/Platform/Rendering/Passes/PostProcessingPass.h"
#include "Core/Platform/Rendering/Passes/UIPass.h"
#include "Core/Platform/Rendering/Passes/VoxelPass.h"
#include "Core/Platform/Rendering/Passes/VoxelBakePass.h"

#include "Core/ECS/Components/VoxRenderer.h"
#include "External/optick/optick.h"

#include "Editor/imgui/Contexts/ImContext.h"
#include "Editor/imgui/Contexts/VKImContext.h"

RenderContext::RenderContext(Platform* pPlatform)
{
	m_pPlatform = pPlatform;
}

void RenderContext::SetFadeValue(float fValue)
{
	m_fFader = fValue;
	m_bFaderUpdated = true;
}

RenderContext::~RenderContext()
{
}

void RenderContext::Initialize()
{
	Settings& settings = m_pPlatform->GetApplication()->GetSettings();
	settings.FullscreenChanged += Event<bool>::Subscriber(std::bind(&RenderContext::OnFullscreenChanged, this, std::placeholders::_1), this);

	m_bIsFullscreen = settings.IsFullscreen();

	const UVector2 initialSize = m_bIsFullscreen
		? m_v2ScreenResolution
		: UVector2(m_pPlatform->GetWindowContext()->GetSize().x, m_pPlatform->GetWindowContext()->GetSize().y);

	m_v2RenderResolution = ConstrainToAspectRatio(initialSize.x, initialSize.y);

	m_pSettings = &m_pPlatform->GetApplication()->GetSettings();

	// Unit debug sphere
#if defined(EDITOR) || defined(_DEBUG)
	m_UnitDebugSphere.reserve(static_cast<size_t>(m_iSphereLineCount * 2.f));

	// Compute our step around each circle
	float twoPi = glm::pi<float>() * 2.f;
	float step = twoPi / m_iSphereResolution;

	// Create the loop on the XY plane first
	for (float a = 0.f; a < twoPi; a += step)
	{
		m_UnitDebugSphere.push_back(Vector3(std::cosf(a), std::sinf(a), 0.f));
		m_UnitDebugSphere.push_back(Vector3(std::cosf(a + step), std::sinf(a + step), 0.f));
	}

	// Next on the XZ plane
	for (float a = 0.f; a < twoPi; a += step)
	{
		m_UnitDebugSphere.push_back(Vector3(std::cosf(a), 0.f, std::sinf(a)));
		m_UnitDebugSphere.push_back(Vector3(std::cosf(a + step), 0.f, std::sinf(a + step)));
	}

	// Finally on the YZ plane
	for (float a = 0.f; a < twoPi; a += step)
	{
		m_UnitDebugSphere.push_back(Vector3(0.f, std::cosf(a), std::sinf(a)));
		m_UnitDebugSphere.push_back(Vector3(0.f, std::cosf(a + step), std::sinf(a + step)));
	}
#endif
}

PRenderContext* RenderContext::Get()
{
	return reinterpret_cast<PRenderContext*>(this);
}

TextureReadData* RenderContext::ReadTexture(const std::string& texturePath)
{
	return m_pTextureManager->ReadTexture(texturePath);
}

void RenderContext::WaitForGPU()
{
	for (auto& it : m_pCommandEngines)
	{
		it.second->WaitForGPU();
	}
}

void RenderContext::Submit(const RenderData& renderData)
{
	m_RenderList.push_back(renderData);
}

void RenderContext::Submit(const DebugLine& renderData)
{
#if defined(EDITOR) || defined(_DEBUG)
	Vector4 color = Vector4(renderData.m_Color.inst.Colors.r, renderData.m_Color.inst.Colors.g, renderData.m_Color.inst.Colors.b, renderData.m_Color.inst.Colors.a) * 0.00392156863f;
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Start, 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_End, 1.f), color });
#endif
}

void RenderContext::Submit(const DebugSphere& renderData)
{
#if defined(EDITOR) || defined(_DEBUG)
	Vector4 color = Vector4(renderData.m_Color.inst.Colors.r, renderData.m_Color.inst.Colors.g, renderData.m_Color.inst.Colors.b, renderData.m_Color.inst.Colors.a) * 0.00392156863f;

	// Create the loop on the XY plane first
	for (size_t i = 0; i < m_UnitDebugSphere.size(); ++i)
	{
		m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + renderData.m_fRadius * m_UnitDebugSphere[i], 1.f), color });
	}
#endif
}

void RenderContext::Submit(const DebugBox& renderData)
{
#if defined(EDITOR) || defined(_DEBUG)
	Vector4 color = Vector4(renderData.m_Color.inst.Colors.r, renderData.m_Color.inst.Colors.g, renderData.m_Color.inst.Colors.b, renderData.m_Color.inst.Colors.a) * 0.00392156863f;

	/* FRONT FACE */
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });

	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, -renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });

	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, -renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, -renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });

	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, -renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });

	/* BACK FACE */
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });

	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, -renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });

	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, -renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, -renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });

	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, -renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });

	/* TOP */
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });

	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });

	/* BOTTOM */
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, -renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(-renderData.m_Extents.x, -renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });

	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, -renderData.m_Extents.y, -renderData.m_Extents.z), 1.f), color });
	m_DebugDrawLines.push_back({ Vector4(renderData.m_Center + Vector3(renderData.m_Extents.x, -renderData.m_Extents.y, renderData.m_Extents.z), 1.f), color });
#endif
}

void RenderContext::Submit(const SpriteData& renderData)
{
	m_SpriteList.push_back(renderData);
}

void RenderContext::Submit(StructuredVoxelBuffer& renderData)
{
	renderData.Distance = glm::distance(Vector3(renderData.Position), Vector3(m_CameraData.m_WorldPos));
	m_AABBList.push_back(renderData);
}

void RenderContext::SortAABBs()
{
	Vector3 invRayDirection = m_CameraData.m_ModelView * Vector4(0.0f, 0.0f, 1.0f, 0.f);
	//Vector3 invRayDirection = Vector3::Transform(Vector3(0.0f, 0.0f, 1.0f), m_CameraData.m_ModelView);
	//CLEANUP

	std::sort(m_AABBList.begin(), m_AABBList.end(),
		[&](const StructuredVoxelBuffer& a, const StructuredVoxelBuffer& b) -> bool
	{
		return a.Distance < b.Distance;
	});
}

void RenderContext::EnableDebugLines(bool bEnabled)
{
	m_bDebugEnabled = bEnabled;
	m_bDebugCleared = false;

#if defined(_DEBUG) || defined(EDITOR)
	ForceUpdate();
#endif
}

bool RenderContext::ResizeWorldBuffer()
{
	Application* pApplication = m_pPlatform->GetApplication();
	World* pWorld = pApplication->GetWorldManager().GetTopWorld();
	PhysicsSystem* pPhysics = pWorld->GetSystem<PhysicsSystem>();
	VoxelGrid* pGrid = pPhysics->GetVoxelGrid();

	UVector3 uWorldSize;
	pGrid->GetDimensions(
		uWorldSize.x,
		uWorldSize.y,
		uWorldSize.z
	);

	// Has changed?
	bool bChanged =  m_pVoxelMapper->Resize(uWorldSize.x * uWorldSize.y * uWorldSize.z, sizeof(uint32_t));
	m_pVoxelData = m_pVoxelMapper->GetData();

	/* The brick grid describes this window, so it follows the same resize.
	   Order matters: the grid drops its mirror pointers first, the mapper is
	   then free to reallocate underneath it, and Flush repopulates. */
	m_BrickGrid.Resize(uWorldSize);
	m_pBrickMapper->Resize(m_BrickGrid.GetBrickCount(), sizeof(uint32_t));
	m_BrickGrid.SetBuffers(m_pBrickMapper->GetData(), m_pBrickMapper->GetBackBufferData());
	m_BrickGrid.Flush();

	/* Whatever the bakers stamped is gone. See GetVoxelGeneration. */
	++m_uiVoxelGeneration;

	return bChanged;
}

uint32_t RenderContext::ValidateBrickGrid()
{
	return m_BrickGrid.Validate(false, m_pVoxelData);
}

uint32_t RenderContext::ValidateFarField()
{
	return FarFieldBaker::Validate(
		m_pPlatform->GetApplication()->GetWorldManager().GetTopWorld(),
		m_FarField
	);
}

UVector3 RenderContext::GetFarFieldShaderGridSize() const
{
	return (m_bFarFieldEnabled && m_FarField.IsBuilt())
		? m_FarField.GetGridSize()
		: UVector3(0, 0, 0);
}

void RenderContext::BuildFarField(World* pWorld)
{
	ChunkSystem* pChunkSystem = pWorld != nullptr ? pWorld->GetChunkSystem() : nullptr;
	PhysicsSystem* pPhysics = pWorld != nullptr ? pWorld->GetSystem<PhysicsSystem>() : nullptr;

	if (pChunkSystem == nullptr || pPhysics == nullptr)
		return;

	/* The level is the chunk grid; the window is at most 3x3 of it. Height is
	   not chunked, so the window's Y is the level's Y. */
	const UVector2 v2LevelXZ = pChunkSystem->GetWorldSize();
	const UVector3 v3WindowSize = pPhysics->GetVoxelGrid()->GetDimensions();
	const UVector3 v3LevelSize(v2LevelXZ.x, v3WindowSize.y, v2LevelXZ.y);

	/* A level the window already covers - a 1x1 chunk grid, which every menu
	   world is - has no far field to draw. Building one would cost the memory
	   and the marching for a volume every ray is masked out of anyway. */
	if (v3LevelSize.x <= v3WindowSize.x && v3LevelSize.z <= v3WindowSize.z)
	{
		m_FarField.Resize(UVector3(0, 0, 0));
		return;
	}

	m_FarField.Resize(v3LevelSize);

	FarFieldBaker::Build(pWorld, m_FarField);

	if (!m_FarField.IsBuilt())
		return;

	/* Same order the window's brick grid is resized in: the grid drops its
	   mirror first, the mapper reallocates, then the mirrors are re-supplied.
	   Only the front buffer exists here - the volume never swaps. */
	m_FarFieldBricks.Resize(m_FarField.GetGridSize());

	m_pFarFieldMapper->Resize(m_FarField.GetCellCount(), sizeof(uint32_t));
	m_pFarFieldBrickMapper->Resize(m_FarFieldBricks.GetBrickCount(), sizeof(uint32_t));

	m_FarFieldBricks.SetBuffers(m_pFarFieldBrickMapper->GetData(), nullptr);

	m_FarField.Flush(m_pFarFieldMapper->GetData(), m_FarFieldBricks);

	ForceCameraDataUpdate();
}

uint32_t RenderContext::GetVoxel(uint32_t uiID) const
{
	return m_pVoxelData[uiID];
}

void RenderContext::ClearVoxels()
{
	memset(m_pVoxelMapper->GetData(), 0, m_pVoxelMapper->GetInfo().m_uiElementCount * m_pVoxelMapper->GetInfo().m_uiElementSize);
	memset(m_pVoxelMapper->GetBackBufferData(), 0, m_pVoxelMapper->GetInfo().m_uiElementCount * m_pVoxelMapper->GetInfo().m_uiElementSize);

	m_BrickGrid.ClearAll();

	++m_uiVoxelGeneration;
}

void RenderContext::Clear()
{
	OPTICK_CATEGORY("Rendercontext", Optick::Category::Rendering);
	OPTICK_EVENT();
	m_RenderList.clear();
	m_AABBList.clear();

#if defined(EDITOR) || defined(_DEBUG)
	m_DebugDrawLines.clear();
#endif
}

void RenderContext::FixedClear()
{
	m_SpriteList.clear();
}

bool RenderContext::Present()
{
	OPTICK_CATEGORY("Rendercontext", Optick::Category::Rendering);
	OPTICK_EVENT();

#ifndef _ORBIS
	// Render ImGui
	ImGui::Render();
#endif

	// Hold timer that counts drawn frames
	float fDeltaTime = static_cast<float>(m_pPlatform->GetApplication()->GetTimer().GetElapsedSeconds());
	m_fFrameTimer += fDeltaTime;

	FrameProfiler::Get().Tick(fDeltaTime);

	/* Worst and 99th-percentile frame of the second, alongside the average.
	   An average cannot tell a stall from a latency problem - 200 fps with one
	   40 ms frame in it reads exactly the same as 200 fps that are all 5 ms,
	   and the two have nothing in common. The percentile is what separates "it
	   hitches now and then" from "every frame is late".
	   Sampled into a fixed ring so this costs a store per frame and no
	   allocation; the profiler is off in Release and this line is not. */
	if (m_uiFrameSamples < k_uiMaxFrameSamples)
		m_fFrameSamples[m_uiFrameSamples++] = fDeltaTime * 1000.f;

	if (m_fFrameTimer >= 1.0f)
	{
		m_fFrameTimer = std::fmod(m_fFrameTimer, 1.0f);

		m_uiFPS = m_uiDrawnFrames;
		m_uiDrawnFrames = 0;

		float fWorst = 0.f;
		float fP99 = 0.f;

		if (m_uiFrameSamples > 0)
		{
			std::sort(m_fFrameSamples, m_fFrameSamples + m_uiFrameSamples);

			fWorst = m_fFrameSamples[m_uiFrameSamples - 1];
			fP99 = m_fFrameSamples[(m_uiFrameSamples * 99) / 100];
		}

		fprintf(stderr, "[fps] %u  (frame ms: p99 %.2f, worst %.2f, over %u samples)\n",
		        m_uiFPS, fP99, fWorst, m_uiFrameSamples);

		m_uiFrameSamples = 0;
	}
	
#if defined(_DEBUG) || defined(EDITOR)
	Buffer* pDebugBuffer = m_mBuffers["Debug Lines"].get();
	PRenderPass* pDebugPass = m_pRenderPasses["Debug Renderer"].get();

	// Draw once with cleared debug line list
	if (!m_bDebugEnabled && !m_bDebugCleared)
	{
		m_DebugDrawLines.clear();
	}
#endif

	Settings& settings = GetPlatform()->GetApplication()->GetSettings();

	// Present
	Buffer* pCameraBuffer = m_mBuffers["Camera Data"].get();
	Buffer* pAABBBuffer = m_mBuffers["AABB Data"].get();

	Buffer* pSpriteBuffer = m_mBuffers["Sprite Data"].get();
	Buffer* pVoxelBakeBuffer = m_mBuffers["Bake Command Data"].get();

	PRenderPass* pParticlePass = m_pRenderPasses["Particles"].get();
	PRenderPass* pVoxelPass = m_pRenderPasses["Voxel"].get();
	PRenderPass* pUIPass = m_pRenderPasses["UI Renderer"].get();
	PRenderPass* pPostProcessingPass = m_pRenderPasses["Post Processing"].get();

	PComputePass* pVoxelBakePass = m_pComputePasses["Voxel Baker"].get();

	PCommandEngine* pVDirectEngine = m_pCommandEngines["VDirect"].get();
	PCommandEngine* pComputeEngine = m_pCommandEngines["Compute"].get();

	PCommandEngine* pDirectEngine = m_pCommandEngines["Direct"].get();

	const bool bIsCompleted = pVDirectEngine->GetCompletedValue() >= pVDirectEngine->GetValue();

	if (bIsCompleted && !m_bIsDrawTextureCopied)
	{
		// Reset command allocators
		if (pDirectEngine->GetValue() > 0)
			pDirectEngine->AdvanceFrame();

		/* Copy target texture to to-be-drawn texture */
		pDirectEngine->Reset();
		pDirectEngine->Start();

		View* pSource = pVoxelPass->GetTargetView();
		pVoxelPass->ToggleBackBuffer();
		View* pTarget = pVoxelPass->GetTargetView();
		pVoxelPass->ToggleBackBuffer();

		// Transition
		pDirectEngine->QueueBarrier(pTarget->GetNative(), E_STATE_COPY_DEST);

		pDirectEngine->QueueBarrier(pSource->GetNative(), E_STATE_COPY_SOURCE);

		pDirectEngine->ApplyBarriers();

		pDirectEngine->CopyResource(pTarget->GetNative(), pSource->GetNative());

		// Transition
		pDirectEngine->QueueBarrier(pTarget->GetNative(), E_STATE_PIXEL_SHADER_RESOURCE);

		pDirectEngine->QueueBarrier(pSource->GetNative(), E_STATE_PIXEL_SHADER_RESOURCE);

		pDirectEngine->ApplyBarriers();

		pDirectEngine->Execute();
		pDirectEngine->AdvanceFrame();

		m_bIsDrawTextureCopied = true;
		m_uiDrawnFrames++;
	}

	// Upload buffers
	if (bIsCompleted) 
	{
		m_uiMissedFrames = 0;
		m_bIsDrawTextureCopied = false;

		Application* pApplication = m_pPlatform->GetApplication();
		World* pWorld = pApplication->GetWorldManager().GetTopWorld();

		/* An editor build loads no world at startup - VoxApp only does that
		   under !EDITOR - so there is nothing to read particles from until one
		   is opened. This was an unconditional dereference. */
		PhysicsSystem* pPhysics = pWorld != nullptr ? pWorld->GetSystem<PhysicsSystem>() : nullptr;

		m_uiParticleCount = pPhysics != nullptr ? pPhysics->m_uiActiveParticleCount : 0;

		// Camera buffer
		{
			UVector2 v2Size = GetRenderResolution();
			v2Size.x *= m_fRenderScale;
			v2Size.y *= m_fRenderScale;

			Vector4 v4LightDirection = glm::normalize(Vector4(-0.4f, -0.8f, 0.6f, 0.0f));

			/* An editor build loads no world at startup, so there is no physics
			   system to take the grid from until one is opened. A zero world
			   size makes the marcher's bounds test fail immediately, which is
			   what "no world" should look like. */
			VoxelGrid* pGrid = pPhysics != nullptr ? pPhysics->GetVoxelGrid() : nullptr;

			UVector3 uWorldSize(0, 0, 0);

			if (pGrid != nullptr)
			{
				pGrid->GetDimensions(
					uWorldSize.x,
					uWorldSize.y,
					uWorldSize.z
				);
			}

			pCameraBuffer->Clear();

			pCameraBuffer->AddConstantData(m_CameraData.m_MVP);
			pCameraBuffer->AddConstantData(m_CameraData.m_ModelView);

			pCameraBuffer->AddConstantData(m_CameraData.m_WorldPos);
			pCameraBuffer->AddConstantData(m_CameraData.m_CameraOffset);

			pCameraBuffer->AddConstantData(Vector4(
				static_cast<float>(v2Size.x), static_cast<float>(v2Size.y),
				m_CameraData.m_bIsOrthographic ? 0 : m_CameraData.m_fProjectionValue, m_CameraData.m_fAspectRatio
			));

			pCameraBuffer->AddConstantData(v4LightDirection);
			pCameraBuffer->AddConstantData(UVector4(uWorldSize, 1.0));

			pCameraBuffer->AddConstantData(settings.GetResolutionScale());
			pCameraBuffer->AddConstantData(m_fFader);

			pCameraBuffer->AddConstantData(m_uiParticleCount);
			pCameraBuffer->AddConstantData(static_cast<uint32_t>(GetAABBList().size()));

			/* Far-field cell grid (RENDERING_PLAN.md phase 4), or zero when
			   there is none - a level the window already covers, a build that
			   found nothing, or the runtime toggle off. FarField.hlsl tests
			   this before touching either far-field mapper, which is what
			   keeps the descriptors valid at one dummy element until a world
			   with a real far field is loaded. */
			pCameraBuffer->AddConstantData(UVector4(GetFarFieldShaderGridSize(), 1));

			/* The camera of the *previous* upload. Post processing samples a
			   copy of the voxel target taken at the top of this frame, so the
			   image it composites was rendered with that camera rather than the
			   one being uploaded now, and anything reconstructing a world-space
			   ray there has to match it - see CameraData.hlsl's sceneInvMvp.
			   Written after the current values and before they are latched, so
			   the buffer carries both. */
			pCameraBuffer->AddConstantData(m_PreviousSceneCamera.m_InvMVP);
			pCameraBuffer->AddConstantData(m_PreviousSceneCamera.m_WorldPos);
			pCameraBuffer->AddConstantData(m_PreviousSceneCamera.m_Offset);

			m_PreviousSceneCamera.m_InvMVP = glm::inverse(m_CameraData.m_MVP);
			m_PreviousSceneCamera.m_WorldPos = m_CameraData.m_WorldPos;
			m_PreviousSceneCamera.m_Offset = m_CameraData.m_CameraOffset;

			pCameraBuffer->Allocate();

			//Sets the forced data update to false
			m_bCameraDataUpdated = false;
			m_bFaderUpdated = false;
		}

		// AABB buffer. Uploaded every frame: the list is rebuilt each frame and
		// entity AABBs move without necessarily setting m_bWorldUpdated, so
		// gating the upload on it rendered from a stale list. It is ~32 bytes
		// per drawn model, so the upload is not worth guarding.
		{
			pAABBBuffer->Clear();
			pAABBBuffer->AddStructuredData(
				m_AABBList.data(),
				sizeof(StructuredVoxelBuffer),
				m_AABBList.size(),
				false
			);
			pAABBBuffer->Allocate();
		}

		if (pVDirectEngine->GetValue() > 0)
		{
			pVDirectEngine->AdvanceFrame();
		}

		pVDirectEngine->Reset();
		pVDirectEngine->Start();

		/* One render pass instance per pass: dynamic rendering cannot nest
		   them, so the DX12-style interleaved Begin order would silently skip
		   every pass after the first. The voxel pass samples the particle
		   targets, so particles draw first. */
		pVDirectEngine->Begin(pParticlePass);
		pVDirectEngine->Draw(pParticlePass);
		pVDirectEngine->End(pParticlePass);

		pVDirectEngine->Begin(pVoxelPass);
		pVDirectEngine->Draw(pVoxelPass);
		pVDirectEngine->End(pVoxelPass);

		pVDirectEngine->ApplyBarriers();

		pVDirectEngine->Execute();
	}

	// Texture data
	{
		pSpriteBuffer->Clear();
		pSpriteBuffer->AddStructuredData(m_SpriteList.data(), sizeof(SpriteData), m_SpriteList.size(), false);
		pSpriteBuffer->Allocate();
	}

#if defined(_DEBUG) || defined(EDITOR)
	// Debug line buffer
	if (m_bDebugEnabled || !m_bDebugCleared)
	{
		pDebugBuffer->Clear();
		pDebugBuffer->AddStructuredData(
			m_DebugDrawLines.data(),
			sizeof(DebugDrawLine),
			m_DebugDrawLines.size(),
			false
		);
		pDebugBuffer->Allocate();
	}
#endif

	if (pDirectEngine->GetCompletedValue() < pDirectEngine->GetValue())
	{
		/* The GPU has not retired the frame we submitted, so there is nothing
		   to do but come back next tick. Sustained, that is indistinguishable
		   from a freeze: the window stops updating while the main loop keeps
		   spinning, and no validation error is produced because nothing
		   illegal happened. Report it once with the numbers that say whether
		   the work is merely enormous or genuinely stuck. */
		++m_uiStalledFrames;

		if (m_uiStalledFrames == 600)
		{
			fprintf(stderr, "[stall] GPU has not completed for 600 frames: "
			                "direct %llu/%llu, vdirect %llu/%llu, voxel instances %u, aabbs %zu\n",
			        static_cast<unsigned long long>(pDirectEngine->GetCompletedValue()),
			        static_cast<unsigned long long>(pDirectEngine->GetValue()),
			        static_cast<unsigned long long>(pVDirectEngine->GetCompletedValue()),
			        static_cast<unsigned long long>(pVDirectEngine->GetValue()),
			        pVoxelPass != nullptr ? pVoxelPass->GetData().m_uiInstanceCount : 0,
			        m_AABBList.size());
		}

		return false;
	}

	m_uiStalledFrames = 0;

	// Reset command allocators
	if (pDirectEngine->GetValue() > 0)
		pDirectEngine->AdvanceFrame();

	pDirectEngine->Reset();

	// Direct Engine List 1
	{
		// pDirectEngine->Wait(pCopyEngine, 1);
		pDirectEngine->Start();

		/* One render pass instance per pass (see the VDirect block above).
		   Post processing samples the UI and debug targets, so both close
		   before it begins. */
		pDirectEngine->Begin(pUIPass);
		pDirectEngine->Draw(pUIPass);
		pDirectEngine->End(pUIPass);

#if defined(_DEBUG) || defined(EDITOR)
		if (m_bDebugEnabled || !m_bDebugCleared)
		{
			pDirectEngine->Begin(pDebugPass);
			pDirectEngine->Draw(pDebugPass);
			pDirectEngine->End(pDebugPass);
		}
#endif

		pDirectEngine->Begin(pPostProcessingPass);
		pDirectEngine->Draw(pPostProcessingPass);

		/* ImContext::Draw takes only the draw data; the Vulkan context reads
		   the command buffer off the engine it was constructed with, so the
		   backend command list no longer has to be threaded through here.
		   It records into the post processing pass's instance. */
		m_pPlatform->GetImguiSystem().GetContext()->Draw(ImGui::GetDrawData());

		pDirectEngine->End(pPostProcessingPass);
		pDirectEngine->ApplyBarriers();

		/* Execute command list */
		pDirectEngine->Execute(); // 1
	}

	if (!m_bDebugEnabled && !m_bDebugCleared)
		m_bDebugCleared = true;

	return true;
}

void RenderContext::InitializeRenderLoop()
{
	Settings& settings = m_pPlatform->GetApplication()->GetSettings();

	VoxelPass* pVoxelPass = nullptr;

	UIPass* pUIPass = nullptr;
	PostProcessingPass* pPostProcessingPass = nullptr;

	Buffer* pCameraBuffer = nullptr;
	Buffer* pAABBBuffer = nullptr;
	Buffer* pBakeCommandBuffer = nullptr;

	Mapper* pParticleMapper = nullptr;
	Buffer* pSpriteBuffer = nullptr;

	Sampler* pLinearSampler = nullptr;
	Sampler* pPointSampler = nullptr;

#if defined(_DEBUG) || defined(EDITOR)
	DebugPass* pDebugPass = nullptr;
	Buffer* pLineBuffer = nullptr;
#endif

	// Camera buffer
	{
		Buffer::Info camBufInfo;
		camBufInfo.m_Name = "Camera Data";
		camBufInfo.m_Type = Buffer::E_CONSTANT;

		m_mBuffers.emplace(camBufInfo.m_Name, std::make_unique<Buffer>(Get(), camBufInfo));
		pCameraBuffer = m_mBuffers[camBufInfo.m_Name].get();
	}

	// Depth buffer
	{
		Buffer::Info aabbBufInfo;
		aabbBufInfo.m_Name = "AABB Data";
		aabbBufInfo.m_Type = Buffer::E_STRUCTURED;

		m_mBuffers.emplace(aabbBufInfo.m_Name, std::make_unique<Buffer>(Get(), aabbBufInfo));
		pAABBBuffer = m_mBuffers[aabbBufInfo.m_Name].get();
	}

	// Bake command buffer
	{
		Buffer::Info bakeCmdBufInfo;
		bakeCmdBufInfo.m_Name = "Bake Command Data";
		bakeCmdBufInfo.m_Type = Buffer::E_CONSTANT;

		m_mBuffers.emplace(bakeCmdBufInfo.m_Name, std::make_unique<Buffer>(Get(), bakeCmdBufInfo));
		pBakeCommandBuffer = m_mBuffers[bakeCmdBufInfo.m_Name].get();
	}

	// Sprite buffer
	{
		Buffer::Info spriteBufInfo;
		spriteBufInfo.m_Name = "Sprite Data";
		spriteBufInfo.m_Type = Buffer::E_STRUCTURED;

		m_mBuffers.emplace(spriteBufInfo.m_Name, std::make_unique<Buffer>(Get(), spriteBufInfo));
		pSpriteBuffer = m_mBuffers[spriteBufInfo.m_Name].get();
	}

#if defined(_DEBUG) || defined(EDITOR)
	// Debug line buffer
	{
		Buffer::Info lineBufInfo;
		lineBufInfo.m_Name = "Debug Lines";
		lineBufInfo.m_Type = Buffer::E_STRUCTURED;

		m_mBuffers.emplace(lineBufInfo.m_Name, std::make_unique<Buffer>(Get(), lineBufInfo));
		pLineBuffer = m_mBuffers[lineBufInfo.m_Name].get();
	}
#endif

	// Samplers
	{
		// Linear sampler
		Sampler::Info linearSamplerDesc;
		linearSamplerDesc.m_FilterMode = E_LINEAR;
		m_pSamplers.push_back(std::make_unique<Sampler>(Get(), linearSamplerDesc));
		pLinearSampler = m_pSamplers.back().get();

		// Point sampler
		Sampler::Info pointSamplerDesc;
		pointSamplerDesc.m_FilterMode = E_POINT;
		m_pSamplers.push_back(std::make_unique<Sampler>(Get(), pointSamplerDesc));
		pPointSampler = m_pSamplers.back().get();
	}

	// 3D voxel mapper
	{
		Mapper::Info voxelMapperDesc;
		voxelMapperDesc.m_Name = "Voxel Data Mapper";
		voxelMapperDesc.m_ColorFormat = E_R8G8B8A8_UNORM;
		voxelMapperDesc.m_GPUAccessType = E_READ_WRITE;

		voxelMapperDesc.m_bHasBackBuffer = true;

		m_pMappers.push_back(std::make_unique<Mapper>(Get(), voxelMapperDesc, false));
		m_pVoxelMapper = m_pMappers.back().get();
		m_pVoxelMapper->BufferSwapped += Event<uint32_t*&>::Subscriber([this](uint32_t*& newData)
		{
			m_pVoxelData = newData;

			/* The brick grid and its mapper describe the voxel window, so they
			   flip with it or they describe the wrong one. Doing it from here
			   rather than at the ChunkSystem call site is what guarantees the
			   three stay in lockstep. */
			m_pBrickMapper->SwapBuffer();
			m_BrickGrid.Swap();
			m_BrickGrid.SetBuffers(m_pBrickMapper->GetData(), m_pBrickMapper->GetBackBufferData());
		}, this);
	}

	/* Occupancy brick mapper (RENDERING_PLAN.md phase 2)
	 *
	 * No colour format, so it binds as a plain storage buffer rather than a
	 * texel buffer - the shader reads raw counts, not texels. Read-write only
	 * so that it lands in the u register range: the t range in front of it is
	 * already occupied by the AABB buffer and the particle textures, and
	 * taking a t register would renumber all of them. Nothing writes to it
	 * from the GPU.
	 *
	 * Back-buffered for the same reason the voxel mapper is - see the
	 * BufferSwapped subscriber above. */
	{
		Mapper::Info brickMapperDesc;
		brickMapperDesc.m_Name = "Voxel Brick Mapper";
		brickMapperDesc.m_ColorFormat = E_UNKNOWN;
		brickMapperDesc.m_GPUAccessType = E_READ_WRITE;

		brickMapperDesc.m_bHasBackBuffer = true;

		m_pMappers.push_back(std::make_unique<Mapper>(Get(), brickMapperDesc, false));
		m_pBrickMapper = m_pMappers.back().get();
	}

	/* Far-field LOD volume (RENDERING_PLAN.md phase 4)
	 *
	 * The whole level at a quarter resolution, so that a ray leaving the 3x3
	 * detail window has something to hit. Static after a build: no back buffer,
	 * because unlike the window it does not slide.
	 *
	 * Colour format matches the voxel mapper's, so the shader reads cells the
	 * same way it reads voxels. Read-write for the same reason the brick mapper
	 * is - it is the u register range that is free, and taking a t would
	 * renumber the textures that are already there. Nothing writes to either
	 * from the GPU. */
	{
		Mapper::Info farFieldDesc;
		farFieldDesc.m_Name = "Far Field Mapper";
		farFieldDesc.m_ColorFormat = E_R8G8B8A8_UNORM;
		farFieldDesc.m_GPUAccessType = E_READ_WRITE;

		m_pMappers.push_back(std::make_unique<Mapper>(Get(), farFieldDesc, false));
		m_pFarFieldMapper = m_pMappers.back().get();

		Mapper::Info farFieldBrickDesc;
		farFieldBrickDesc.m_Name = "Far Field Brick Mapper";
		farFieldBrickDesc.m_ColorFormat = E_UNKNOWN;
		farFieldBrickDesc.m_GPUAccessType = E_READ_WRITE;

		m_pMappers.push_back(std::make_unique<Mapper>(Get(), farFieldBrickDesc, false));
		m_pFarFieldBrickMapper = m_pMappers.back().get();

		/* One element each so the descriptors are valid before a world has been
		   loaded. GetFarFieldShaderGridSize reports (0,0,0) until a build
		   succeeds, and the shader skips the far field entirely on that. */
		m_pFarFieldMapper->Resize(1, sizeof(uint32_t));
		m_pFarFieldBrickMapper->Resize(1, sizeof(uint32_t));
	}

	// Particle Pass
	{
		// Vertex shader
		Shader::Info vertexShader;
		vertexShader.m_FilePath = "Engine/Assets/Shaders/Particles.vs";
		vertexShader.m_Type = Shader::E_VERTEX;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), vertexShader));
		Shader* pVertexShader = m_pShaders.back().get();

		// Pixel shader
		Shader::Info pixelShader;
		pixelShader.m_FilePath = "Engine/Assets/Shaders/Particles.ps";
		pixelShader.m_Type = Shader::E_PIXEL;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), pixelShader));
		Shader* pPixelShader = m_pShaders.back().get();

		// Particle Mapper
		Mapper::Info mapperInfo;
		mapperInfo.m_Name = "Particle Mapper";
		mapperInfo.m_ColorFormat = E_UNKNOWN;

		m_pMappers.push_back(std::make_unique<Mapper>(Get(), mapperInfo, false));
		pParticleMapper = m_pMappers.back().get();

		// Create screen render target from data
		m_pParticlePass = new ParticlePass(Get(), pVertexShader, pPixelShader, pCameraBuffer, pParticleMapper, pPointSampler);
		m_pRenderPasses.emplace(m_pParticlePass->GetData().m_Name, std::unique_ptr<ParticlePass>(m_pParticlePass));
	}

	// Voxel Pass
	{
		// Vertex shader
		Shader::Info vertexShader;
		vertexShader.m_FilePath = "Engine/Assets/Shaders/VoxelRenderer.vs";
		vertexShader.m_Type = Shader::E_VERTEX;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), vertexShader));
		Shader* pVertexShader = m_pShaders.back().get();

		// Pixel shader
		Shader::Info pixelShader;
		pixelShader.m_FilePath = settings.IsShadowEnabled() ? "Engine/Assets/Shaders/VoxelRenderer.ps" : "Engine/Assets/Shaders/VoxelRenderer.ShadowLess.ps";
		pixelShader.m_Type = Shader::E_PIXEL;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), pixelShader));
		Shader* pPixelShader = m_pShaders.back().get();

		// Create screen render target from data
		pVoxelPass = new VoxelPass(Get(), pVertexShader, pPixelShader, pPointSampler, m_pVoxelMapper, m_pBrickMapper, pCameraBuffer, pAABBBuffer, m_pParticlePass->GetTargetView(0), m_pParticlePass->GetTargetView(1));
		m_pRenderPasses.emplace(pVoxelPass->GetData().m_Name, std::unique_ptr<VoxelPass>(pVoxelPass));
	}

#if defined(_DEBUG) || defined(EDITOR)
	// Debug
	{
		// Vertex shader
		Shader::Info vertexShader;
		vertexShader.m_FilePath = "Engine/Assets/Shaders/Debug.vs";
		vertexShader.m_Type = Shader::E_VERTEX;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), vertexShader));
		Shader* pVertexShader = m_pShaders.back().get();

		// Pixel shader
		Shader::Info pixelShader;
		pixelShader.m_FilePath = "Engine/Assets/Shaders/Debug.ps";
		pixelShader.m_Type = Shader::E_PIXEL;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), pixelShader));
		Shader* pPixelShader = m_pShaders.back().get();

		// Create screen render target from data
		pDebugPass = new DebugPass(Get(), pVertexShader, pPixelShader, pCameraBuffer, pLineBuffer);
		m_pRenderPasses.emplace(pDebugPass->GetData().m_Name, std::unique_ptr<DebugPass>(pDebugPass));
	}
#endif

	// UI
	{
		// Vertex shader
		Shader::Info vertexShader;
		vertexShader.m_FilePath = "Engine/Assets/Shaders/UIRenderer.vs";
		vertexShader.m_Type = Shader::E_VERTEX;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), vertexShader));
		Shader* pVertexShader = m_pShaders.back().get();

		// Pixel shader
		Shader::Info pixelShader;
		pixelShader.m_FilePath = "Engine/Assets/Shaders/UIRenderer.ps";
		pixelShader.m_Type = Shader::E_PIXEL;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), pixelShader));
		Shader* pPixelShader = m_pShaders.back().get();

		// Create screen render target from data
		pUIPass = new UIPass(Get(), pVertexShader, pPixelShader, pLinearSampler, pCameraBuffer, pSpriteBuffer);
		m_pRenderPasses.emplace(pUIPass->GetData().m_Name, std::unique_ptr<UIPass>(pUIPass));
	}

	// Post Processing
	{
		// Vertex shader
		Shader::Info vertexShader;
		vertexShader.m_FilePath = "Engine/Assets/Shaders/ScreenQuad.vs";
		vertexShader.m_Type = Shader::E_VERTEX;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), vertexShader));
		Shader* pVertexShader = m_pShaders.back().get();

		// Pixel shader
		Shader::Info pixelShader;
#if defined(_DEBUG) || defined(EDITOR)
		pixelShader.m_FilePath = "Engine/Assets/Shaders/PostProcessing.Debug.ps";
#else
		pixelShader.m_FilePath = "Engine/Assets/Shaders/PostProcessing.ps";
#endif
		pixelShader.m_Type = Shader::E_PIXEL;

		m_pShaders.push_back(std::make_unique<Shader>(Get(), pixelShader));
		Shader* pPixelShader = m_pShaders.back().get();

		// To 1, needed for GetTargetView
		pVoxelPass->ToggleBackBuffer();

		// Create screen render target from data
		pPostProcessingPass = new PostProcessingPass(
			Get(),
			pVertexShader,
			pPixelShader,
			pLinearSampler,
			pCameraBuffer,
			m_pVoxelMapper,
			m_pFarFieldMapper,
			m_pFarFieldBrickMapper,
#if defined(_DEBUG) || defined(EDITOR)
			{ pVoxelPass->GetTargetView(), pUIPass->GetTargetView(), pDebugPass->GetTargetView() }
#else
			{ pVoxelPass->GetTargetView(), pUIPass->GetTargetView() }
#endif
		);

		// To 0
		pVoxelPass->ToggleBackBuffer();

		m_pRenderPasses.emplace(pPostProcessingPass->GetData().m_Name, std::unique_ptr<PostProcessingPass>(pPostProcessingPass));
	}
}

void RenderContext::LoadTexture(TextureReference* pTextureReference)
{
	if (!pTextureReference || pTextureReference->TextureView)
		return;

	m_pTextureManager->LoadTexture(m_pCommandEngines["Texture"]->Get(), pTextureReference);
}

void RenderContext::DestroyTexture(const TextureReference* pTextureRef)
{
	WaitForGPU();
	m_pTextureManager->DestroyTexture(pTextureRef);
}

void RenderContext::OnFullscreenChanged(bool bFullscreen)
{
	m_bIsFullscreen = bFullscreen;
	FullscreenChanged(bFullscreen);

	if (bFullscreen)
	{
		OnResize(m_v2ScreenResolution.x, m_v2ScreenResolution.y);
	}
	else
	{
		/* SDL owns the window on every desktop platform now, so there is no
		   Win32 branch here any more: GetHandle() is an SDL_Window*, and the
		   old code cast it to an HWND for GetClientRect. */
		OnResize(m_v2RenderResolution.x, m_v2RenderResolution.y);
	}
}

UVector2 RenderContext::ConstrainToAspectRatio(uint32_t uiWidth, uint32_t uiHeight) const
{
	const float fLocked =
		m_pPlatform->GetApplication()->GetSettings().GetLockedAspectRatio();

	if (fLocked <= 0.f || uiWidth == 0 || uiHeight == 0)
		return UVector2(uiWidth, uiHeight);

	/* Largest box of the locked ratio that fits the window; the leftover is
	   the letterbox the swapchain blit leaves black. */
	const float fWindow = static_cast<float>(uiWidth) / static_cast<float>(uiHeight);

	if (fWindow > fLocked)
		uiWidth = static_cast<uint32_t>(uiHeight * fLocked);
	else
		uiHeight = static_cast<uint32_t>(uiWidth / fLocked);

	return UVector2(std::max(uiWidth, 1u), std::max(uiHeight, 1u));
}

Vector2 RenderContext::WindowToRenderNormalized(const Vector2& v2WindowPoint) const
{
	const UVector2 windowSize = m_pPlatform->GetWindowContext()->GetSize();

	/* m_v2RenderResolution is the constrained size in window pixels; the
	   present blit centres it. */
	const float fBarX = (static_cast<float>(windowSize.x) - static_cast<float>(m_v2RenderResolution.x)) * 0.5f;
	const float fBarY = (static_cast<float>(windowSize.y) - static_cast<float>(m_v2RenderResolution.y)) * 0.5f;

	if (m_v2RenderResolution.x == 0 || m_v2RenderResolution.y == 0)
		return Vector2(0.f);

	return Vector2(
		(v2WindowPoint.x - fBarX) / static_cast<float>(m_v2RenderResolution.x),
		(v2WindowPoint.y - fBarY) / static_cast<float>(m_v2RenderResolution.y));
}

bool RenderContext::OnResize(uint32_t uiWidth, uint32_t uiHeight)
{
	const UVector2 constrained = ConstrainToAspectRatio(uiWidth, uiHeight);
	uiWidth = constrained.x;
	uiHeight = constrained.y;

	if (m_v2RenderResolution.x == uiWidth && m_v2RenderResolution.y == uiHeight)
		return false;

	UVector2 oldResolution = m_v2RenderResolution;
	m_v2RenderResolution = UVector2(uiWidth, uiHeight);

	IVector2 resolutionDelta;
	resolutionDelta.x = static_cast<int32_t>(m_v2RenderResolution.x * m_fRenderScale) - static_cast<int32_t>(oldResolution.x * m_fRenderScale);
	resolutionDelta.y = static_cast<int32_t>(m_v2RenderResolution.y * m_fRenderScale) - static_cast<int32_t>(oldResolution.y * m_fRenderScale);

	SizeChanged(
		static_cast<uint32_t>(uiWidth * m_fRenderScale),
		static_cast<uint32_t>(uiHeight * m_fRenderScale),
		resolutionDelta
	);

	return true;
}
