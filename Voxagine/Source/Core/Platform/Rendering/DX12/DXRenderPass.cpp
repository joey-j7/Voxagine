#include "pch.h"

#include "Core/Platform/Rendering/DX12/DXRenderPass.h"
#include "Core/Platform/Rendering/RenderPass.h"
#include "Core/Platform/Rendering/RenderDefines.h"

#include "Core/Platform/Rendering/DX12/DX12RenderContext.h"
#include "Core/Platform/Rendering/Objects/Buffer.h"
#include "Core/Platform/Rendering/Objects/Shader.h"
#include "Core/Platform/Rendering/Objects/View.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"
#include "Core/Platform/Rendering/Objects/Sampler.h"

#include "Core/Platform/Rendering/DX12/DXCommandEngine.h"
#include "Core/Platform/Rendering/DX12/Managers/DXHeapManager.h"

#include "DXHelper.h"
#include "External/DirectX12/d3dx12.h"

#include <xlocbuf>
#include <codecvt>

void DXRenderPass::Init(const RenderPass::Data& data)
{
	m_Data = data;

	m_fInvRenderScale = 1.0f / m_Data.m_fRenderScale;

	ID3D12Device* pDevice = m_pContext->GetDevice();

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring nameW = converter.from_bytes(m_Data.m_Name);

	// Only when textures resources are bound
	size_t count1 = m_Data.m_Textures.size() + m_Data.m_Mappers.size() + m_Data.m_uiBindlessResourceCount;

	for (PRenderPass* pPass : m_Data.m_PassOutput)
	{
		count1 += pPass->GetData().m_uiRenderViewCount;
	}

	size_t parameterCount = m_Data.m_Buffers.size() + count1;

	uint32_t uiHeapCount = static_cast<uint32_t>(count1);
	uint32_t uiHeapBufferCount = 0;

	for (auto& pBuffer : m_Data.m_Buffers)
	{
		if (pBuffer->GetInfo().m_Type != Buffer::E_CONSTANT)
		{
			if (pBuffer->GetInfo().m_GPUAccessType == E_READ_ONLY)
			{
				uiHeapCount++;
				uiHeapBufferCount++;
			}
		}
	}

	if (!m_pHeapManager && uiHeapCount > 0)
	{
		m_pHeapManager = new DXHeapManager(m_pContext, nameW + L" SRV", uiHeapCount);
		m_bIsHeapOwner = true;
	}

	// Create pipeline
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		;

		CD3DX12_DESCRIPTOR_RANGE1* ranges = new CD3DX12_DESCRIPTOR_RANGE1[parameterCount];
		CD3DX12_ROOT_PARAMETER1* rootParameters = new CD3DX12_ROOT_PARAMETER1[parameterCount];

		uint32_t uiCBVCount = 0;
		uint32_t uiSRVCount = 0;
		uint32_t uiUAVCount = 0;
		
		for (Buffer* pBuffer : m_Data.m_Buffers)
		{
			uint32_t uiViewID = GetViewID(pBuffer->GetInfo().m_Name);

			if (pBuffer->GetInfo().m_Type == Buffer::E_CONSTANT)
			{
				ranges[uiViewID].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, uiCBVCount, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
				rootParameters[uiViewID].InitAsConstantBufferView(uiCBVCount, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
				uiCBVCount++;
			}
			else if (pBuffer->GetInfo().m_GPUAccessType == E_READ_WRITE)
			{
				ranges[uiViewID].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, uiUAVCount, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
				rootParameters[uiViewID].InitAsUnorderedAccessView(uiUAVCount, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_ALL);
				uiUAVCount++;
			}
			else
			{
				ranges[uiViewID].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, uiSRVCount, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
				rootParameters[uiViewID].InitAsShaderResourceView(uiSRVCount, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_ALL);
				uiSRVCount++;
			}
		}

		for (Mapper* pMapper : m_Data.m_Mappers)
		{
			uint32_t uiViewID = GetViewID(pMapper->GetInfo().m_Name);

			if (pMapper->GetInfo().m_GPUAccessType == E_READ_WRITE)
			{
				m_uiHeapIDs.push_back(m_pHeapManager->ReserveResource());
				ranges[uiViewID].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, uiUAVCount, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

				if (pMapper->GetInfo().m_bHasBackBuffer)
				{
					m_uiHeapIDs.push_back(m_pHeapManager->ReserveResource());
				}

				if (pMapper->GetInfo().m_ColorFormat == E_UNKNOWN)
				{
					rootParameters[uiViewID].InitAsUnorderedAccessView(static_cast<UINT>(m_uiHeapIDs.size() - 1), 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_ALL);
				}
				else
				{
					rootParameters[uiViewID].InitAsDescriptorTable(1, &ranges[uiViewID], D3D12_SHADER_VISIBILITY_ALL);
				}

				uiUAVCount++;
			}
			else
			{
				m_uiHeapIDs.push_back(m_pHeapManager->ReserveResource());
				ranges[uiViewID].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, uiSRVCount, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

				if (pMapper->GetInfo().m_bHasBackBuffer)
				{
					m_uiHeapIDs.push_back(m_pHeapManager->ReserveResource());
				}

				if (pMapper->GetInfo().m_ColorFormat == E_UNKNOWN)
				{
					rootParameters[uiViewID].InitAsShaderResourceView(static_cast<UINT>(m_uiHeapIDs.size() - 1), 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_ALL);
				}
				else
				{
					rootParameters[uiViewID].InitAsDescriptorTable(1, &ranges[uiViewID], D3D12_SHADER_VISIBILITY_PIXEL);
				}

				uiSRVCount++;
			}
		}

		for (uint32_t i = 0; i < m_Data.m_PassOutput.size(); ++i)
		{
			PRenderPass* pPass = m_Data.m_PassOutput[i];

			for (uint32_t j = 0; j < pPass->GetData().m_uiRenderViewCount; ++j)
			{
				uint32_t uiViewID = GetViewID("Pass " + std::to_string(i) + " View " + std::to_string(j));
				m_uiHeapIDs.push_back(m_pHeapManager->ReserveResource());

				ranges[uiViewID].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, uiSRVCount, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
				rootParameters[uiViewID].InitAsDescriptorTable(1, &ranges[uiViewID], D3D12_SHADER_VISIBILITY_PIXEL);

				uiSRVCount++;
			}
		}

		for (View* pTexture : m_Data.m_Textures)
		{
			uint32_t uiViewID = GetViewID(pTexture->GetInfo().m_Name);
			m_uiHeapIDs.push_back(m_pHeapManager->ReserveResource());

			ranges[uiViewID].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, uiSRVCount, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
			rootParameters[uiViewID].InitAsDescriptorTable(1, &ranges[uiViewID], D3D12_SHADER_VISIBILITY_PIXEL);

			uiSRVCount++;
		}

		for (uint32_t i = 0; i < m_Data.m_uiBindlessResourceCount; ++i)
		{
			uint32_t uiViewID = GetViewID(("Unbounded Resource " + std::to_string(i)).c_str());

			ranges[uiViewID].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, uiSRVCount, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
			rootParameters[uiViewID].InitAsDescriptorTable(1, &ranges[uiViewID]);

			uiSRVCount++;
		}

		CD3DX12_STATIC_SAMPLER_DESC* samplers = new CD3DX12_STATIC_SAMPLER_DESC[m_Data.m_Samplers.size()];

		for (uint32_t i = 0; i < m_Data.m_Samplers.size(); ++i)
		{
			const Sampler::Info& samplerInfo = m_Data.m_Samplers[i]->GetInfo();

			samplers[i].Filter = static_cast<D3D12_FILTER>(samplerInfo.m_FilterMode);
			samplers[i].AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(samplerInfo.m_WrapMode);
			samplers[i].AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(samplerInfo.m_WrapMode);
			samplers[i].AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(samplerInfo.m_WrapMode);
			samplers[i].MipLODBias = 0;
			samplers[i].MaxAnisotropy = 1;
			samplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			samplers[i].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			samplers[i].MinLOD = 0.0f;
			samplers[i].MaxLOD = D3D12_FLOAT32_MAX;
			samplers[i].ShaderRegister = i;
			samplers[i].RegisterSpace = 0;
			samplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		}
		
		// Root parameter descriptor
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(static_cast<UINT>(parameterCount), rootParameters, static_cast<UINT>(m_Data.m_Samplers.size()), samplers, rootSignatureFlags);

		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		Microsoft::WRL::ComPtr<ID3DBlob> error;

		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));

		ThrowIfFailed(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)));
		m_pRootSignature->SetName(nameW.c_str());

		delete[] samplers;
		delete[] ranges;
		delete[] rootParameters;
	}

	// Create descriptor heaps.
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NumDescriptors = (m_Data.m_TargetType != D3D12_RESOURCE_STATE_PRESENT) ? m_Data.m_uiRenderViewCount + m_Data.m_uiBackBuffers * m_Data.m_uiRenderViewCount : RenderContext::m_uiFrameCount;

		// Describe and create a render target view (RTV) descriptor heap.
		ThrowIfFailed(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap)));
		m_uiRTVDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Only if depth testing is enabled
		if (m_Data.m_bEnableDepth)
		{
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			dsvHeapDesc.NumDescriptors = 1;

			// Describe and create a shader resource view (SRV) heap for the texture.
			ThrowIfFailed(pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSVHeap)));
			m_uiDSVDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			if (m_Data.m_DepthFormat != DXGI_FORMAT_UNKNOWN)
			{
				// Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
				// on this surface.
				View::Info depthInfo;
				depthInfo.m_Name = "Depth Texture";
				depthInfo.m_ColorFormat = m_Data.m_DepthFormat;
				depthInfo.m_State = E_STATE_DEPTH_WRITE;
				depthInfo.m_Size = UVector3(GetTargetSize(), 1.0f);
				depthInfo.m_Type = View::E_DEPTH_STENCIL_VIEW;

				m_pDepthView = std::make_unique<View>(m_pContext, depthInfo);
				m_pDepthView->AddTarget(this, 0, View::E_DEPTH_STENCIL_VIEW);
				m_pDSVHeap->SetName((nameW + L" DSV").c_str());
			}
		}
	}

	// Create RTV(s) for regular render targets
	if (m_Data.m_TargetType != D3D12_RESOURCE_STATE_PRESENT)
	{
		for (uint32_t i = 0; i < m_Data.m_uiRenderViewCount; ++i)
		{
			for (uint32_t j = 0; j < m_Data.m_uiBackBuffers + 1; ++j)
			{
				// Create a RTV for single resource
				View::Info textureInfo;
				textureInfo.m_Name = m_Data.m_Name + " " + std::to_string(i) + " " + std::to_string(j);
				textureInfo.m_Size = UVector3(GetTargetSize(), 0.0f);
				textureInfo.m_ColorFormat = m_Data.m_TargetFormat[i];
				textureInfo.m_Type = View::E_RENDER_TARGET_VIEW;

				m_pTargetViews.push_back(std::make_unique<View>(m_pContext, textureInfo));
				m_pTargetViews.back()->AddTarget(this, i + j * m_Data.m_uiRenderViewCount, View::E_RENDER_TARGET_VIEW);
			}
		}
	}
	// Create RTVs for swapchain render targets
	else
	{
		// These should always use the screen resolution
		assert(m_Data.m_bUseScreenResolution);
		m_Data.m_bUseScreenResolution = true;

		// There should always be one instance
		assert(m_Data.m_uiRenderViewCount == 1);
		m_Data.m_uiRenderViewCount = 1;

		// Create a RTV for each frame.
		for (UINT n = 0; n < RenderContext::m_uiFrameCount; n++)
		{
			m_pTargetViews.push_back(std::make_unique<View>(m_pContext));
			std::string name2 = m_Data.m_Name + " " + std::to_string(n);
			m_pTargetViews.back()->m_Info.m_Name = name2;
			m_pTargetViews.back()->m_Info.m_Type = View::E_RENDER_TARGET_VIEW;

			ID3D12Resource*& pResource = m_pTargetViews[n]->GetNative();
			ThrowIfFailed(m_pContext->GetSwapChain()->GetBuffer(n, IID_PPV_ARGS(&pResource)));

			m_pTargetViews.back()->AddTarget(this, n, View::E_RENDER_TARGET_VIEW);
		}
	}

	uint32_t heapID = 0;

	// Create SRV/UAV for each mapper
	{
		for (Mapper* pMapper : m_Data.m_Mappers)
		{
			// TODO: might be incorrect heapID for backbuffer
			pMapper->AddTarget(this, m_uiHeapIDs[heapID]);
			heapID += pMapper->GetInfo().m_bHasBackBuffer ? 2 : 1;
		}
	}

	// Create SRV/UAV for each texture view
	{
		for (PRenderPass* pPass : m_Data.m_PassOutput)
		{
			for (uint32_t i = 0; i < pPass->GetData().m_uiRenderViewCount; ++i)
			{
				for (uint32_t j = 0; j < pPass->GetData().m_uiBackBuffers + 1; ++j)
				{
					View* pTexture = pPass->m_pTargetViews[i + j * pPass->GetData().m_uiRenderViewCount].get();
					pTexture->AddTarget(this, m_uiHeapIDs[heapID], View::E_SHADER_RESOURCE_VIEW);
				}

				heapID++;
			}
		}

		for (View* pTexture : m_Data.m_Textures)
		{
			pTexture->AddTarget(this, m_uiHeapIDs[heapID], View::E_SHADER_RESOURCE_VIEW);
			heapID++;
		}
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		// Define the vertex input layout.
		// D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		// {
		// 	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		// 	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		// };

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { m_Data.m_VertexLayout.data(), static_cast<UINT>(m_Data.m_VertexLayout.size()) };
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_Data.m_pVertexShader->GetNative());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_Data.m_pPixelShader->GetNative());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(m_Data.m_CullType);
		psoDesc.RasterizerState.AntialiasedLineEnable = true;

		if (m_Data.m_BlendEnabled) {
			D3D12_BLEND_DESC alphaBlend = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

			alphaBlend.RenderTarget[0].BlendEnable = TRUE;

			alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			alphaBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;

			alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			alphaBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

			alphaBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			alphaBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

			alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0x0f;

			psoDesc.BlendState = alphaBlend;
		}
		else{
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		}
		
		/*psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;*/

		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state
		psoDesc.DepthStencilState.DepthEnable = m_Data.m_bEnableDepth;

		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(m_Data.m_TopologyType);
		psoDesc.NumRenderTargets = m_Data.m_uiRenderViewCount;
		psoDesc.DSVFormat = static_cast<DXGI_FORMAT>(m_Data.m_DepthFormat);

		for (uint32_t i = 0; i < m_Data.m_uiRenderViewCount; ++i)
		{
			psoDesc.RTVFormats[i] = static_cast<DXGI_FORMAT>(m_Data.m_TargetFormat[i]);
		}

		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState)));
	}
}

DXRenderPass::~DXRenderPass()
{
	for (uint32_t uiID : m_uiHeapIDs)
	{
		m_pHeapManager->FreeResource(uiID);
	}

	if (m_bIsHeapOwner)
		delete m_pHeapManager;

	/* TODO: Remove RTV handle */
}

void DXRenderPass::Begin(PCommandEngine* pEngine)
{
	const bool bHasIndices = m_Data.m_uiIndexCount > 0;
	const bool bHasVertices = m_Data.m_uiVertexCount > 0;
	const bool bHasInstances = m_Data.m_uiInstanceCount > 0;

	if (!m_bIsDrawn && (!bHasInstances || (!bHasVertices && !bHasIndices)))
		return;

	// Indicate that the resource will now be used as a render target
	if (m_Data.m_TargetType != E_STATE_RENDER_TARGET)
	{
		// Only set current frame buffer state
		if (m_Data.m_TargetType == E_STATE_PRESENT)
		{
			PResourceStates oldState = m_pTargetViews[m_pContext->GetFrameIndex()]->SetState(pEngine, E_STATE_RENDER_TARGET);

			// Makes the texture common for use with other command queues
			pEngine->m_Barriers.push_back(
				CD3DX12_RESOURCE_BARRIER::Transition(
					m_pTargetViews[m_pContext->GetFrameIndex()]->GetNative(),
					static_cast<D3D12_RESOURCE_STATES>(oldState),
					static_cast<D3D12_RESOURCE_STATES>(E_STATE_RENDER_TARGET)
				)
			);
		}
		else
		{
			for (uint32_t i = 0; i < m_Data.m_uiRenderViewCount; ++i)
			{
				uint32_t id = i + m_uiCurrentBackBuffer * m_Data.m_uiRenderViewCount;
				PResourceStates oldState = m_pTargetViews[id]->SetState(pEngine, E_STATE_RENDER_TARGET);

				pEngine->m_Barriers.push_back(
					CD3DX12_RESOURCE_BARRIER::Transition(
						m_pTargetViews[id]->GetNative(),
						static_cast<D3D12_RESOURCE_STATES>(oldState),
						static_cast<D3D12_RESOURCE_STATES>(E_STATE_RENDER_TARGET)
					)
				);
			}
		}
	}
}

void DXRenderPass::Draw(PCommandEngine* pEngine)
{
	const bool bHasIndices = m_Data.m_uiIndexCount > 0;
	const bool bHasVertices = m_Data.m_uiVertexCount > 0;
	const bool bHasInstances = m_Data.m_uiInstanceCount > 0;

	if (!bHasInstances || (!bHasVertices && !bHasIndices))
	{
		if (m_bIsDrawn)
		{
			Clear(pEngine);

			m_bIsDrawn = false;
			m_bIsCleared = true;
		}

		return;
	}

	ID3D12GraphicsCommandList* pCommandList = pEngine->GetList();

	// Set pipeline state
	pCommandList->SetPipelineState(m_pPipelineState.Get());

	// Set necessary state
	pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

	if (m_pHeapManager)
	{
		// Set descriptor heaps
		ID3D12DescriptorHeap* heaps[] = { m_pHeapManager->GetNative() };
		pCommandList->SetDescriptorHeaps(1, heaps);
	}

	// Set viewport
	if (m_Data.m_TargetType == D3D12_RESOURCE_STATE_PRESENT)
	{
		UVector2 uRenderSize = m_pContext->GetRenderResolution();
		D3D12_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(uRenderSize.x), static_cast<float>(uRenderSize.y), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
		D3D12_RECT rect = { 0, 0, static_cast<LONG>(uRenderSize.x), static_cast<LONG>(uRenderSize.y) };

		pCommandList->RSSetViewports(1, &vp);
		pCommandList->RSSetScissorRects(1, &rect);
	}
	else
	{
		UVector2 uRenderSize = GetTargetSize();
		D3D12_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(uRenderSize.x), static_cast<float>(uRenderSize.y), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
		D3D12_RECT rect = { 0, 0, static_cast<LONG>(uRenderSize.x), static_cast<LONG>(uRenderSize.y) };

		pCommandList->RSSetViewports(1, &vp);
		pCommandList->RSSetScissorRects(1, &rect);
	}

	//Get the render target count based on being swap chain or not
	uint32_t uiRTCount = m_Data.m_uiRenderViewCount;

	//Create RTV handles
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvHandles;

	for (uint32_t i = 0; i < uiRTCount; i++)
	{
		rtvHandles.push_back(GetRTVCPUHandle(m_Data.m_TargetType == D3D12_RESOURCE_STATE_PRESENT ? m_pContext->GetFrameIndex() : (i + m_uiCurrentBackBuffer * m_Data.m_uiRenderViewCount)));
	}

	/*CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRTVCPUHandle(
		m_Data.m_TargetType == D3D12_RESOURCE_STATE_PRESENT ? m_pContext->GetFrameIndex() : 0
	);*/

	if (m_Data.m_bEnableDepth)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDSVHeap->GetCPUDescriptorHandleForHeapStart();
		pCommandList->OMSetRenderTargets(uiRTCount, rtvHandles.data(), FALSE, &dsvHandle);
	}
	else
	{
		pCommandList->OMSetRenderTargets(uiRTCount, rtvHandles.data(), FALSE, nullptr);
	}

	// Clear
	if (m_Data.m_bClearPerFrame)
	{
		for (uint32_t i = 0; i < uiRTCount; ++i) {
			const float fClearColor[4] = { m_Data.m_TargetFormat[i] == E_R32_FLOAT ? (m_Data.m_ClearColor.r) : (m_Data.m_ClearColor.r, m_Data.m_ClearColor.g, m_Data.m_ClearColor.b, m_Data.m_ClearColor.a) };
			uint32_t id = i + m_uiCurrentBackBuffer * m_Data.m_uiRenderViewCount;
			pCommandList->ClearRenderTargetView(rtvHandles[id], fClearColor, 0, nullptr);
		}

		if (m_Data.m_bEnableDepth)
		{
			pCommandList->ClearDepthStencilView(
				m_pDSVHeap->GetCPUDescriptorHandleForHeapStart(),
				D3D12_CLEAR_FLAG_DEPTH, m_Data.m_DepthClearValue, 0, 0, nullptr
			);
		}
	}

	// Record commands.
	pCommandList->IASetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(m_Data.m_Topology));

	// Setup the texture and its sampler on the PS stage
	for (Buffer* pBuffer : m_Data.m_Buffers)
	{
		uint32_t uiViewID = GetViewID(pBuffer->GetInfo().m_Name);
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = pBuffer->GetGPUAddress();

		if (pBuffer->GetInfo().m_GPUAccessType == E_READ_WRITE)
		{
			pCommandList->SetGraphicsRootUnorderedAccessView(uiViewID, gpuAddress);
		}
		else
		{
			switch (pBuffer->GetInfo().m_Type)
			{
			case Buffer::E_CONSTANT:	pCommandList->SetGraphicsRootConstantBufferView(uiViewID, gpuAddress);	break;
			case Buffer::E_STRUCTURED:	pCommandList->SetGraphicsRootShaderResourceView(uiViewID, gpuAddress);	break;
			default:					assert(false);															break;
			}
		}
	}

	uint32_t heapID = 0;

	for (Mapper* pMapper : m_Data.m_Mappers)
	{
		uint32_t viewID = GetViewID(pMapper->GetInfo().m_Name);

		if (pMapper->GetInfo().m_ColorFormat != E_UNKNOWN)
		{
			pCommandList->SetGraphicsRootDescriptorTable(viewID, m_pHeapManager->GetGPUHandle(m_uiHeapIDs[heapID + pMapper->GetBackBufferIndex()]));
		}
		else if (pMapper->GetInfo().m_GPUAccessType == E_READ_WRITE)
		{
			pCommandList->SetGraphicsRootUnorderedAccessView(viewID, pMapper->GetNative()->GetGPUVirtualAddress());
		}
		else
		{
			pCommandList->SetGraphicsRootShaderResourceView(viewID, pMapper->GetNative()->GetGPUVirtualAddress());
		}

		heapID += pMapper->GetInfo().m_bHasBackBuffer ? 2 : 1;
	}

	for (uint32_t i = 0; i < m_Data.m_PassOutput.size(); ++i)
	{
		PRenderPass* pPass = m_Data.m_PassOutput[i];

		for (uint32_t j = 0; j < pPass->GetData().m_uiRenderViewCount; ++j)
		{
			uint32_t viewID = GetViewID("Pass " + std::to_string(i) + " View " + std::to_string(j));
			pCommandList->SetGraphicsRootDescriptorTable(viewID, m_pHeapManager->GetGPUHandle(m_uiHeapIDs[heapID]));

			heapID++;
		}
	}

	for (View* pTexture : m_Data.m_Textures)
	{
		uint32_t viewID = GetViewID(pTexture->GetInfo().m_Name);
		pCommandList->SetGraphicsRootDescriptorTable(viewID, m_pHeapManager->GetGPUHandle(m_uiHeapIDs[heapID]));

		heapID++;
	}

	for (uint32_t i = 0; i < m_Data.m_uiBindlessResourceCount; ++i)
	{
		uint32_t viewID = GetViewID(("Unbounded Resource " + std::to_string(i)).c_str());
		pCommandList->SetGraphicsRootDescriptorTable(viewID, m_pHeapManager->GetGPUHandle(0));

		heapID++;
	}

	D3D12_VERTEX_BUFFER_VIEW vertexView = { 0, 0, 0 };
	D3D12_INDEX_BUFFER_VIEW indexView = { 0, 0, DXGI_FORMAT_R32_UINT };
	
	pCommandList->IASetVertexBuffers(0, 0, &vertexView);
	pCommandList->IASetIndexBuffer(&indexView);

	pCommandList->DrawInstanced(m_Data.m_uiVertexCount, m_Data.m_uiInstanceCount, 0, 0);
	m_bIsDrawn = true;
}

void DXRenderPass::End(PCommandEngine* pEngine)
{
	const bool bHasIndices = m_Data.m_uiIndexCount > 0;
	const bool bHasVertices = m_Data.m_uiVertexCount > 0;
	const bool bHasInstances = m_Data.m_uiInstanceCount > 0;

	if ((!m_bIsDrawn && !m_bIsCleared) && (!bHasInstances || (!bHasVertices && !bHasIndices)))
		return;

	// Indicate that the resource will now be used as a render target
	if (m_Data.m_TargetType != E_STATE_RENDER_TARGET)
	{
		// Only set current frame buffer state
		if (m_Data.m_TargetType == E_STATE_PRESENT)
		{
			PResourceStates oldState = m_pTargetViews[m_pContext->GetFrameIndex()]->SetState(pEngine, m_Data.m_TargetType);

			// Makes the texture common for use with other command queues
			pEngine->m_Barriers.push_back(
				CD3DX12_RESOURCE_BARRIER::Transition(
					m_pTargetViews[m_pContext->GetFrameIndex()]->GetNative(),
					static_cast<D3D12_RESOURCE_STATES>(oldState),
					static_cast<D3D12_RESOURCE_STATES>(m_Data.m_TargetType)
				)
			);
		}
		else
		{
			for (uint32_t i = 0; i < m_Data.m_uiRenderViewCount; ++i)
			{
				uint32_t id = i + m_uiCurrentBackBuffer * m_Data.m_uiRenderViewCount;
				PResourceStates oldState = m_pTargetViews[id]->SetState(pEngine, m_Data.m_TargetType);

				pEngine->m_Barriers.push_back(
					CD3DX12_RESOURCE_BARRIER::Transition(
						m_pTargetViews[id]->GetNative(),
						static_cast<D3D12_RESOURCE_STATES>(oldState),
						static_cast<D3D12_RESOURCE_STATES>(m_Data.m_TargetType)
					)
				);
			}
		}
	}

	m_bIsCleared = false;
}

void DXRenderPass::Clear(PCommandEngine* pEngine)
{
	ID3D12GraphicsCommandList* pCommandList = pEngine->GetList();

	// Get the render target count based on being swap chain or not
	uint32_t uiRTCount = m_Data.m_uiRenderViewCount;

	// Create RTV handles
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvHandles;

	for (uint32_t i = 0; i < uiRTCount; i++)
	{
		rtvHandles.push_back(GetRTVCPUHandle(m_Data.m_TargetType == D3D12_RESOURCE_STATE_PRESENT ? m_pContext->GetFrameIndex() : (i + m_uiCurrentBackBuffer * m_Data.m_uiRenderViewCount)));
	}

	// Clear render targets
	for (uint32_t i = 0; i < uiRTCount; ++i) {
		const float fClearColor[4] = { m_Data.m_TargetFormat[i] == E_R32_FLOAT ? (m_Data.m_ClearColor.r) : (m_Data.m_ClearColor.r, m_Data.m_ClearColor.g, m_Data.m_ClearColor.b, m_Data.m_ClearColor.a) };
		pCommandList->ClearRenderTargetView(rtvHandles[i], fClearColor, 0, nullptr);
	}

	if (m_Data.m_bEnableDepth)
	{
		pCommandList->ClearDepthStencilView(
			m_pDSVHeap->GetCPUDescriptorHandleForHeapStart(),
			D3D12_CLEAR_FLAG_DEPTH, m_Data.m_DepthClearValue, 0, 0, nullptr
		);
	}
}

View* DXRenderPass::GetTargetView(uint32_t i) const
{
	return m_pTargetViews[i + m_uiCurrentBackBuffer * m_Data.m_uiRenderViewCount].get();
}

View* DXRenderPass::GetDepthView() const
{
	return m_pDepthView.get();
}

void DXRenderPass::Resize(UVector2 uSize)
{
	if (!m_Data.m_bUseScreenResolution)
		return;

	m_Data.m_TargetSize = GetTargetSize();

	// Render target textures only
	if (m_Data.m_TargetType != D3D12_RESOURCE_STATE_PRESENT)
	{
		for (uint32_t i = 0; i < m_pTargetViews.size(); ++i)
		{
			uint32_t id = i + m_uiCurrentBackBuffer * m_Data.m_uiRenderViewCount;
			m_pTargetViews[id]->Resize(UVector3(m_Data.m_TargetSize, 1.0f));
		}
	}
	// Swap buffer textures will be handled by the swap chain, release them so the swap chain can fill them
	else
	{
		for (uint32_t i = 0; i < m_pTargetViews.size(); ++i)
		{
			if (m_pTargetViews[i]->GetNative())
				m_pTargetViews[i]->GetNative()->Release();
		}
	}

	if (m_Data.m_bEnableDepth)
		m_pDepthView->Resize(UVector3(m_Data.m_TargetSize, 1.0f));
}

UVector2 DXRenderPass::GetTargetSize() const
{
	if (m_Data.m_bUseScreenResolution)
	{
		UVector2 uRenderResolution = m_pContext->GetRenderResolution();
		return UVector2(m_Data.m_fRenderScale * uRenderResolution.x, m_Data.m_fRenderScale * uRenderResolution.y);
	}

	return UVector2(m_Data.m_fRenderScale * m_Data.m_TargetSize.x, m_Data.m_fRenderScale * m_Data.m_TargetSize.y);
}

uint32_t DXRenderPass::GetViewID(const std::string& sName)
{
	auto find = m_mViewIDs.find(sName);
	if (find == m_mViewIDs.end()) {
		uint32_t uiSize = static_cast<uint32_t>(m_mViewIDs.size());
		auto descriptor = m_mViewIDs.emplace(sName, uiSize);
		return uiSize;
	}

	return find->second;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DXRenderPass::GetRTVCPUHandle(uint32_t uiID)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), uiID, m_uiRTVDescriptorSize);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DXRenderPass::GetDSVCPUHandle(uint32_t uiID)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart(), uiID, m_uiDSVDescriptorSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DXRenderPass::GetRTVGPUHandle(uint32_t uiID)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetGPUDescriptorHandleForHeapStart(), uiID, m_uiRTVDescriptorSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DXRenderPass::GetDSVGPUHandle(uint32_t uiID)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pDSVHeap->GetGPUDescriptorHandleForHeapStart(), uiID, m_uiDSVDescriptorSize);
}