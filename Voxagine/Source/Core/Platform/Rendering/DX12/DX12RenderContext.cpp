#include "pch.h"

#include <cstdlib>

#include "DX12RenderContext.h"
#include "DXHelper.h"

#include "Core/Platform/Platform.h"
#include "Core/Platform/Window/WindowContext.h"
#include "Core/Platform/Rendering/DX12/DXCommandEngine.h"

/* Object */
#include "Core/Platform/Rendering/Objects/Shader.h"
#include "Core/Platform/Rendering/Objects/View.h"
#include "Core/Platform/Rendering/Objects/Buffer.h"
#include "Core/Platform/Rendering/Objects/Sampler.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

/* Retrieving world size */
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include "Core/ECS/World.h"
#include "Core/ECS/WorldManager.h"
#include "Core/Application.h"

#include "Core/Platform/Rendering/Managers/TextureManagerInc.h"

#include "Core/Platform/Rendering/RenderDefines.h"
#include "Core/Platform/Rendering/RenderContext.h"

#include "Editor/imgui/Contexts/DXImContext.h"

DX12RenderContext::DX12RenderContext(Platform* pPlatform) : RenderContext(pPlatform)
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper Initialize(RO_INIT_MULTITHREADED);
	ThrowIfFailed(Initialize);

	/* Get screen resolution*/
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();

	GetWindowRect(hDesktop, &desktop);
	m_v2ScreenResolution = UVector2(desktop.right, desktop.bottom);
}

DX12RenderContext::~DX12RenderContext()
{
}

void RenderContext::Report()
{
#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	}
#endif
}

void DX12RenderContext::Initialize()
{
	RenderContext::Initialize();

	m_pSettings = &m_pPlatform->GetApplication()->GetSettings();

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> pDebugController;

		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
		{
			pDebugController->EnableDebugLayer();

			//ComPtr<ID3D12Debug1> debugController1;
			//if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(debugController1.GetAddressOf()))))
			//{
			//	debugController1->SetEnableGPUBasedValidation(true);
			//	debugController1->SetEnableSynchronizedCommandQueueValidation(true);
			//}
		}
		else
		{
			OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
		}

		Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
		{
			dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_pDXGIFactory.ReleaseAndGetAddressOf())));

	// Determines whether tearing support is available for fullscreen borderless windows.
	if (m_pSettings->IsTearingEnabled())
	{
		BOOL allowTearing = FALSE;

		Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
		HRESULT hr = m_pDXGIFactory.As(&factory5);
		if (SUCCEEDED(hr))
		{
			hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		}

		if (FAILED(hr) || !allowTearing)
		{
			m_pSettings->SetTearing(false);
#ifdef _DEBUG
			OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
		}
	}

	if (m_bUseWarpDevice)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(m_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		DXGI_ADAPTER_DESC desc;
		warpAdapter->GetDesc(&desc);
		m_pSettings->SetGPUName(desc.Description);

		ThrowIfFailed(
			D3D12CreateDevice(
				warpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_pDevice)
			)
		);
	}
	else
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(&hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_pDevice)
		));
	}

#ifndef NDEBUG
	// Configure debug device (if active).
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> d3dInfoQueue;
	if (SUCCEEDED(m_pDevice.As(&d3dInfoQueue)))
	{
#ifdef _DEBUG
		d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		d3dInfoQueue->AddStorageFilterEntries(&filter);
	}
#endif

	// Get window size
	WindowContext* pWindow = m_pPlatform->GetWindowContext();
	HWND windowHandle = *(HWND*)pWindow->GetHandle();

	// Create command engines
	m_pCommandEngines.emplace("Copy", new PCommandEngine(this, { CommandEngine::E_COPY, "Copy" }));
	m_pCommandEngines.emplace("Direct", new PCommandEngine(this, { CommandEngine::E_DIRECT, "Direct" }));
	m_pCommandEngines.emplace("Texture", new PCommandEngine(this, { CommandEngine::E_DIRECT, "Texture" }));
	m_pCommandEngines.emplace("VDirect", new PCommandEngine(this, { CommandEngine::E_DIRECT, "VDirect" }));
	m_pCommandEngines.emplace("Compute", new PCommandEngine(this, { CommandEngine::E_COMPUTE, "Compute" }));

	// m_pCommandEngines.emplace(CommandEngine::E_COMPUTE,	pCopyEngine);

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = m_uiFrameCount;
	swapChainDesc.Width = GetRenderResolution().x;
	swapChainDesc.Height = GetRenderResolution().y;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = m_pSettings->IsTearingEnabled() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = TRUE;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(
		m_pDXGIFactory->CreateSwapChainForHwnd(
			static_cast<DXCommandEngine*>(m_pCommandEngines["Direct"].get())->GetQueue(),        // Swap chain needs the queue so that it can force a flush on it.
			windowHandle,
			&swapChainDesc,
			&fsSwapChainDesc,
			nullptr,
			&swapChain
		)
	);

	m_pTextureManager = std::make_unique<PTextureManager>(this);
	m_pModelManager = std::make_unique<PModelManager>(this);

	// Disable standard full screen transition on purpose, we have a custom way of doing this
	ThrowIfFailed(m_pDXGIFactory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_pSwapChain));
	m_uiFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	InitializeRenderLoop();

	if (m_pSettings->IsFullscreen()) {
		OnFullscreenChanged(true);
	}
}

void DX12RenderContext::Deinitialize()
{

}

void DX12RenderContext::Clear()
{
	RenderContext::Clear();

	// Clearing is done in command engines
}

bool DX12RenderContext::Present()
{
	if (RenderContext::Present())
	{
		// Present the frame.
		/* Check if VSync or tearing is enabled */
		const UINT uiVSync = m_pSettings->IsVSyncEnabled() ? 1 : 0;
		const UINT uiTearing = m_pSettings->IsTearingEnabled() && !uiVSync ? DXGI_PRESENT_ALLOW_TEARING : 0;

		ThrowIfFailed(m_pSwapChain->Present(uiVSync, uiTearing), GetDevice());

		// Set current swap chain back buffer index
		m_uiFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
		
		m_bWorldUpdated = false;

		return true;
	}
	
	return false;
}

bool DX12RenderContext::OnResize(uint32_t uiWidth, uint32_t uiHeight)
{
	if (!RenderContext::OnResize(uiWidth, uiHeight))
		return false;

	for (auto& pEngine : m_pCommandEngines)
	{
		pEngine.second->WaitForGPU();
	}

	for (auto& target : m_pRenderPasses)
	{
		PRenderPass* rt = target.second.get();
		rt->Resize(UVector2(m_v2RenderResolution.x * m_fRenderScale, m_v2RenderResolution.y * m_fRenderScale));
	}

	m_pSwapChain->ResizeBuffers(
		m_uiFrameCount,
		static_cast<uint32_t>(m_v2RenderResolution.x * m_fRenderScale),
		static_cast<uint32_t>(m_v2RenderResolution.y * m_fRenderScale),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_pSettings->IsTearingEnabled() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
	);

	// Set native screen textures to swap chain
	for (auto& target : m_pRenderPasses)
	{
		PRenderPass* rt = target.second.get();

		if (rt->GetData().m_TargetType == E_STATE_PRESENT)
		{
			for (uint32_t i = 0; i < RenderContext::m_uiFrameCount; ++i)
			{
				ThrowIfFailed(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&rt->GetTargetView(i)->GetNative())));
				rt->GetTargetView(i)->AddTarget(rt, i, View::E_RENDER_TARGET_VIEW);
			}
		}
	}

	// Set current swap chain back buffer index
	m_uiFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	return true;
}

void DX12RenderContext::GetHardwareAdapter(IDXGIAdapter1** ppAdapter)
{
	*ppAdapter = nullptr;

	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
	Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
	HRESULT hr = m_pDXGIFactory.As(&factory6);

	if (SUCCEEDED(hr))
	{
		for (UINT adapterIndex = 0;
			DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()));
			adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
#ifdef _DEBUG
				wchar_t buff[256] = {};
				swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
				OutputDebugStringW(buff);
#endif
				break;
			}
		}
	}
	else
#endif
		for (UINT adapterIndex = 0;
			DXGI_ERROR_NOT_FOUND != m_pDXGIFactory->EnumAdapters1(
				adapterIndex,
				adapter.ReleaseAndGetAddressOf());
			++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		ThrowIfFailed(adapter->GetDesc1(&desc));

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE || desc.VendorId == 5140)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
#ifdef _DEBUG
			wchar_t buff[256] = {};
			swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
			OutputDebugStringW(buff);
#endif
			break;
		}
	}

#if !defined(NDEBUG)
	if (!adapter)
	{
		// Try WARP12 instead
		if (FAILED(m_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
		{
			throw std::exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
		}

		OutputDebugStringA("Direct3D Adapter - WARP12\n");
	}
#endif

	if (!adapter)
	{
		throw std::exception("No Direct3D 12 device found");
	}

	DXGI_ADAPTER_DESC1 desc;
	adapter->GetDesc1(&desc);
	m_pSettings->SetGPUName(desc.Description);

	*ppAdapter = adapter.Detach();
}
