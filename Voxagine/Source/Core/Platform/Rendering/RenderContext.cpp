#include "pch.h"
#include "RenderContext.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Settings.h"

#include "Core/Platform/Window/WindowContext.h"

#include "Core/ECS/World.h"
#include "Core/ECS/WorldManager.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"

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

#ifdef _WINDOWS
#include <eventtoken.h>
#include "Editor/imgui/Contexts/DXImContext.h"
#endif

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
	m_v2RenderResolution = m_bIsFullscreen ? m_v2ScreenResolution : UVector2(m_pPlatform->GetWindowContext()->GetSize().x, m_pPlatform->GetWindowContext()->GetSize().y);

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

	return bChanged;
}

uint32_t RenderContext::GetVoxel(uint32_t uiID) const
{
	return m_pVoxelData[uiID];
}

void RenderContext::ClearVoxels()
{
	memset(m_pVoxelMapper->GetData(), 0, m_pVoxelMapper->GetInfo().m_uiElementCount * m_pVoxelMapper->GetInfo().m_uiElementSize);
	memset(m_pVoxelMapper->GetBackBufferData(), 0, m_pVoxelMapper->GetInfo().m_uiElementCount * m_pVoxelMapper->GetInfo().m_uiElementSize);
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

	if (m_fFrameTimer >= 1.0f)
	{
		m_fFrameTimer = std::fmod(m_fFrameTimer, 1.0f);

		m_uiFPS = m_uiDrawnFrames;
		m_uiDrawnFrames = 0;
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

	const bool bIsCompleted = pVDirectEngine->GetFence()->GetCompletedValue() >= pVDirectEngine->GetValue();

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
		pDirectEngine->m_Barriers.push_back(
			CD3DX12_RESOURCE_BARRIER::Transition(
				pTarget->GetNative(),
				static_cast<D3D12_RESOURCE_STATES>(E_STATE_PIXEL_SHADER_RESOURCE),
				static_cast<D3D12_RESOURCE_STATES>(E_STATE_COPY_DEST)
			)
		);

		pDirectEngine->m_Barriers.push_back(
			CD3DX12_RESOURCE_BARRIER::Transition(
				pSource->GetNative(),
				static_cast<D3D12_RESOURCE_STATES>(E_STATE_PIXEL_SHADER_RESOURCE),
				static_cast<D3D12_RESOURCE_STATES>(E_STATE_COPY_SOURCE)
			)
		);

		pDirectEngine->ApplyBarriers();

		pDirectEngine->GetList()->CopyResource(pTarget->GetNative(), pSource->GetNative());

		// Transition
		pDirectEngine->m_Barriers.push_back(
			CD3DX12_RESOURCE_BARRIER::Transition(
				pTarget->GetNative(),
				static_cast<D3D12_RESOURCE_STATES>(E_STATE_COPY_DEST),
				static_cast<D3D12_RESOURCE_STATES>(E_STATE_PIXEL_SHADER_RESOURCE)
			)
		);

		pDirectEngine->m_Barriers.push_back(
			CD3DX12_RESOURCE_BARRIER::Transition(
				pSource->GetNative(),
				static_cast<D3D12_RESOURCE_STATES>(E_STATE_COPY_SOURCE),
				static_cast<D3D12_RESOURCE_STATES>(E_STATE_PIXEL_SHADER_RESOURCE)
			)
		);

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
		PhysicsSystem* pPhysics = pWorld->GetSystem<PhysicsSystem>();

		m_uiParticleCount = pPhysics->m_uiActiveParticleCount;

		// Camera buffer
		{
			UVector2 v2Size = GetRenderResolution();
			v2Size.x *= m_fRenderScale;
			v2Size.y *= m_fRenderScale;

			Vector4 v4LightDirection = glm::normalize(Vector4(-0.4f, -0.8f, 0.6f, 0.0f));

			VoxelGrid* pGrid = pPhysics->GetVoxelGrid();

			UVector3 uWorldSize;
			pGrid->GetDimensions(
				uWorldSize.x,
				uWorldSize.y,
				uWorldSize.z
			);

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

			pCameraBuffer->AddConstantData(pPhysics->m_uiActiveParticleCount);
			pCameraBuffer->AddConstantData(static_cast<uint32_t>(GetAABBList().size()));

			pCameraBuffer->Allocate();

			//Sets the forced data update to false
			m_bCameraDataUpdated = false;
			m_bFaderUpdated = false;
		}

		// AABB buffer
#ifndef _ORBIS
		if (m_bWorldUpdated)
#endif
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

		/* Barrier towards render target state */
		pVDirectEngine->Begin(pParticlePass);
		pVDirectEngine->Begin(pVoxelPass);

		pVDirectEngine->ApplyBarriers();

		pVDirectEngine->Draw(pParticlePass);

		pVDirectEngine->End(pParticlePass);

		pVDirectEngine->ApplyBarriers();

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

	if (pDirectEngine->GetFence()->GetCompletedValue() < pDirectEngine->GetValue())
		return false;

	// Reset command allocators
	if (pDirectEngine->GetValue() > 0)
		pDirectEngine->AdvanceFrame();

	pDirectEngine->Reset();

	// Direct Engine List 1
	{
		// pDirectEngine->Wait(pCopyEngine, 1);
		pDirectEngine->Start();

		pDirectEngine->Begin(pPostProcessingPass);
		pDirectEngine->Begin(pUIPass);

#if defined(_DEBUG) || defined(EDITOR)
		if (m_bDebugEnabled || !m_bDebugCleared)
		{
			pDirectEngine->Begin(pDebugPass);
		}
#endif

		pDirectEngine->ApplyBarriers();

		pDirectEngine->Draw(pUIPass);

#if defined(_DEBUG) || defined(EDITOR)
		if (m_bDebugEnabled || !m_bDebugCleared)
		{
			pDirectEngine->Draw(pDebugPass);
		}
#endif

#if defined(_DEBUG) || defined(EDITOR)
		if (m_bDebugEnabled || !m_bDebugCleared)
		{
			pDirectEngine->End(pDebugPass);
		}
#endif

		pDirectEngine->End(pUIPass);
		pDirectEngine->ApplyBarriers();

		pDirectEngine->Draw(pPostProcessingPass);

#ifndef _ORBIS
		static_cast<DXImContext*>(
			m_pPlatform->GetImguiSystem().GetContext()
		)->Draw(
			ImGui::GetDrawData(),
			static_cast<DXCommandEngine*>(pDirectEngine)->GetList()
		);
#endif

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
		}, this);
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
		pVoxelPass = new VoxelPass(Get(), pVertexShader, pPixelShader, pPointSampler, m_pVoxelMapper, pCameraBuffer, pAABBBuffer, m_pParticlePass->GetTargetView(0), m_pParticlePass->GetTargetView(1));
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
#ifdef _WINDOWS
		RECT Rect;
		GetClientRect(*(HWND*)m_pPlatform->GetWindowContext()->GetHandle(), &Rect);

		OnResize(Rect.right - Rect.left, Rect.bottom - Rect.top);
#else
		OnResize(m_v2RenderResolution.x, m_v2RenderResolution.y);
#endif
	}
}

bool RenderContext::OnResize(uint32_t uiWidth, uint32_t uiHeight)
{
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
