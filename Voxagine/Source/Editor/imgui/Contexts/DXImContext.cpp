#include "pch.h"
#include "DXImContext.h"

#include "External/imgui/imgui.h"

#include <D3Dcompiler.h>
#include <d3dcommon.h>

#include "Core/Platform/Rendering/DX12/DX12RenderContext.h"
#include "Core/Application.h"

#include <pix_win.h>
#include <pix.h>

#include "Core/Platform/Rendering/Managers/TextureManagerInc.h"
#include "Core/Platform/Rendering/DX12/Managers/DXHeapManager.h"
#include "Core/Platform/Rendering/DX12/DXHelper.h"

// DirectX data
ID3D12Device* DXImContext::m_pD3dDevice = NULL;
ID3D10Blob* DXImContext::m_pVertexShaderBlob = NULL;
ID3D10Blob* DXImContext::m_pPixelShaderBlob = NULL;
ID3D12RootSignature* DXImContext::m_pRootSignature = NULL;
ID3D12PipelineState* DXImContext::m_pPipelineState = NULL;
DXGI_FORMAT DXImContext::m_RTVFormat = DXGI_FORMAT_UNKNOWN;
ID3D12Resource* DXImContext::m_pFontTextureResource = NULL;
D3D12_CPU_DESCRIPTOR_HANDLE  DXImContext::m_FontSrvCpuDescHandle = {};
D3D12_GPU_DESCRIPTOR_HANDLE  DXImContext::m_FontSrvGpuDescHandle = {};

FrameResources* DXImContext::m_pFrameResources = NULL;
UINT DXImContext::m_uiNumFramesInFlight = 0;
UINT DXImContext::m_uiFrameIndex = UINT_MAX;

DXImContext::DXImContext(DX12RenderContext* pContext)
{
	/* Assign variables */
	m_pContext = pContext;
	m_pD3dDevice = pContext->GetDevice();
	m_RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Reserve SRV handle
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NumDescriptors = 1;

	m_uiNumFramesInFlight = RenderContext::m_uiFrameCount;
	m_pFrameResources = new FrameResources[m_uiNumFramesInFlight];
	m_uiFrameIndex = UINT_MAX;

	// Create buffers with a default size (they will later be grown as needed)
	for (UINT i = 0; i < m_uiNumFramesInFlight; ++i)
	{
		m_pFrameResources[i].IB = NULL;
		m_pFrameResources[i].VB = NULL;
		m_pFrameResources[i].VertexBufferSize = 5000;
		m_pFrameResources[i].IndexBufferSize = 10000;
	}
}

void DXImContext::NewFrame()
{
	if (!m_pPipelineState)
		CreateDeviceObjects();
}

void DXImContext::Draw(ImDrawData* drawData, ID3D12GraphicsCommandList* pCommandList)
{
	m_pCommandList = pCommandList;
	Draw(drawData);
}

void DXImContext::Draw(ImDrawData* drawData)
{
	ID3D12DescriptorHeap* heaps[] = { m_pContext->GetTextureManager()->GetHeapManager()->GetNative() };
	m_pCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

	UVector2 renderRes = m_pContext->GetRenderResolution();
	
	PIXBeginEvent(m_pCommandList, PIX_COLOR_DEFAULT, L"Draw imgui");
	
	// FIXME: I'm assuming that this only gets called once per frame!
	// If not, we can't just re-allocate the IB or VB, we'll have to do a proper allocator.
	m_uiFrameIndex += 1;
	
	FrameResources* frameResources = &m_pFrameResources[m_uiFrameIndex % m_uiNumFramesInFlight];
	ID3D12Resource* g_pVB = frameResources->VB;
	ID3D12Resource* g_pIB = frameResources->IB;
	int g_VertexBufferSize = frameResources->VertexBufferSize;
	int g_IndexBufferSize = frameResources->IndexBufferSize;
	
	// Create and grow vertex/index buffers if needed
	if (!g_pVB || g_VertexBufferSize < drawData->TotalVtxCount)
	{
		if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
		g_VertexBufferSize = drawData->TotalVtxCount + 5000;
		D3D12_HEAP_PROPERTIES props;
		memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
		props.Type = D3D12_HEAP_TYPE_UPLOAD;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		D3D12_RESOURCE_DESC desc;
		memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = g_VertexBufferSize * sizeof(ImDrawVert);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (m_pD3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&g_pVB)) < 0)
			return;
		frameResources->VB = g_pVB;
		frameResources->VertexBufferSize = g_VertexBufferSize;
	}
	if (!g_pIB || g_IndexBufferSize < drawData->TotalIdxCount)
	{
		if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
		g_IndexBufferSize = drawData->TotalIdxCount + 10000;
		D3D12_HEAP_PROPERTIES props;
		memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
		props.Type = D3D12_HEAP_TYPE_UPLOAD;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		D3D12_RESOURCE_DESC desc;
		memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = g_IndexBufferSize * sizeof(ImDrawIdx);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (m_pD3dDevice->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&g_pIB)) < 0)
			return;
		frameResources->IB = g_pIB;
		frameResources->IndexBufferSize = g_IndexBufferSize;
	}
	
	// Copy and convert all vertices into a single contiguous buffer
	void* vtx_resource, *idx_resource;
	D3D12_RANGE range;
	memset(&range, 0, sizeof(D3D12_RANGE));
	if (g_pVB->Map(0, &range, &vtx_resource) != S_OK)
		return;
	if (g_pIB->Map(0, &range, &idx_resource) != S_OK)
		return;
	ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource;
	ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource;
	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = drawData->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	g_pVB->Unmap(0, &range);
	g_pIB->Unmap(0, &range);
	
	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). 
	VERTEX_CONSTANT_BUFFER vertex_constant_buffer;
	{
		VERTEX_CONSTANT_BUFFER* constant_buffer = &vertex_constant_buffer;
		float L =drawData->DisplayPos.x;
		float R =drawData->DisplayPos.x + drawData->DisplaySize.x;
		float T =drawData->DisplayPos.y;
		float B =drawData->DisplayPos.y + drawData->DisplaySize.y;
		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
	}
	
	// Setup viewport
	D3D12_VIEWPORT vp;
	memset(&vp, 0, sizeof(D3D12_VIEWPORT));
	vp.Width = drawData->DisplaySize.x;
	vp.Height = drawData->DisplaySize.y;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = vp.TopLeftY = 0.0f;
	m_pCommandList->RSSetViewports(1, &vp);
	
	// Bind shader and vertex buffers
	uint32_t stride = sizeof(ImDrawVert);
	uint32_t offset = 0;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	memset(&vbv, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	vbv.BufferLocation = g_pVB->GetGPUVirtualAddress() + offset;
	vbv.SizeInBytes = g_VertexBufferSize * stride;
	vbv.StrideInBytes = stride;
	m_pCommandList->IASetVertexBuffers(0, 1, &vbv);
	D3D12_INDEX_BUFFER_VIEW ibv;
	memset(&ibv, 0, sizeof(D3D12_INDEX_BUFFER_VIEW));
	ibv.BufferLocation = g_pIB->GetGPUVirtualAddress();
	ibv.SizeInBytes = g_IndexBufferSize * sizeof(ImDrawIdx);
	ibv.Format = sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	m_pCommandList->IASetIndexBuffer(&ibv);
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->SetPipelineState(m_pPipelineState);
	m_pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	m_pCommandList->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);
	
	// Setup render state
	const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
	m_pCommandList->OMSetBlendFactor(blend_factor);
	
	// Render command lists
	int vtx_offset = 0;
	int idx_offset = 0;
	ImVec2 pos = drawData->DisplayPos;
	for (int n = 0; n < drawData->CmdListsCount; ++n)
	{
		const ImDrawList* cmd_list = drawData->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				const D3D12_RECT r = { (LONG)(pcmd->ClipRect.x - pos.x), (LONG)(pcmd->ClipRect.y - pos.y), (LONG)(pcmd->ClipRect.z - pos.x), (LONG)(pcmd->ClipRect.w - pos.y) };
				m_pCommandList->SetGraphicsRootDescriptorTable(1, *(D3D12_GPU_DESCRIPTOR_HANDLE*)&pcmd->TextureId);
				m_pCommandList->RSSetScissorRects(1, &r);
				m_pCommandList->DrawIndexedInstanced(pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
			}
			idx_offset += pcmd->ElemCount;
		}
		vtx_offset += cmd_list->VtxBuffer.Size;
	}

	PIXEndEvent(m_pCommandList);
}

bool DXImContext::CreateDeviceObjects()
{
	if (!m_pD3dDevice)
		return false;

	if (m_pPipelineState)
		InvalidateDeviceObjects();

	// Create the root signature
	{
		D3D12_DESCRIPTOR_RANGE descRange = {};
		descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descRange.NumDescriptors = 1;
		descRange.BaseShaderRegister = 0;
		descRange.RegisterSpace = 0;
		descRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER param[2] = {};

		param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		param[0].Constants.ShaderRegister = 0;
		param[0].Constants.RegisterSpace = 0;
		param[0].Constants.Num32BitValues = 16;
		param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[1].DescriptorTable.NumDescriptorRanges = 1;
		param[1].DescriptorTable.pDescriptorRanges = &descRange;
		param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC staticSampler = {};
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.MipLODBias = 0.f;
		staticSampler.MaxAnisotropy = 0;
		staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticSampler.MinLOD = 0.f;
		staticSampler.MaxLOD = 0.f;
		staticSampler.ShaderRegister = 0;
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = _countof(param);
		desc.pParameters = param;
		desc.NumStaticSamplers = 1;
		desc.pStaticSamplers = &staticSampler;
		desc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		ID3DBlob* blob = NULL;
		if (D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, NULL) != S_OK)
			return false;

		m_pD3dDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
		blob->Release();
	}

	// By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
	// If you would like to use this DX12 sample code but remove this dependency you can: 
	//  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
	//  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL. 
	// See https://github.com/ocornut/imgui/pull/638 for sources and details.

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.NodeMask = 1;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.pRootSignature = m_pRootSignature;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_RTVFormat;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	// Create the vertex shader
	{
		const std::string& engineAssetsPath = m_pContext->GetPlatform()->GetApplication()->GetSettings().GetEngineAssetsPath() + "/Shaders/imgui.vs";
		std::wstring wEngineAssetsPath(engineAssetsPath.begin(), engineAssetsPath.end());
		HRESULT hr = D3DCompileFromFile(wEngineAssetsPath.c_str(), NULL, NULL, "main", "vs_5_0", 0, 0, &m_pVertexShaderBlob, NULL);

		if (FAILED(hr)) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
		{
			printf("Failed compiling vertex shader %08X\n", hr);
			return false;
		}

		psoDesc.VS = { m_pVertexShaderBlob->GetBufferPointer(), m_pVertexShaderBlob->GetBufferSize() };

		// Create the input layout
		static D3D12_INPUT_ELEMENT_DESC local_layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert*)0)->col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		psoDesc.InputLayout = { local_layout, 3 };
	}

	// Create the pixel shader
	{
		const std::string& engineAssetsPath = m_pContext->GetPlatform()->GetApplication()->GetSettings().GetEngineAssetsPath() + "/Shaders/imgui.ps";
		std::wstring wEngineAssetsPath(engineAssetsPath.begin(), engineAssetsPath.end());
		HRESULT hr = D3DCompileFromFile(wEngineAssetsPath.c_str(), NULL, NULL, "main", "ps_5_0", 0, 0, &m_pPixelShaderBlob, NULL);

		if (FAILED(hr)) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
		{
			printf("Failed compiling pixel shader %08X\n", hr);
			return false;
		}

		psoDesc.PS = { m_pPixelShaderBlob->GetBufferPointer(), m_pPixelShaderBlob->GetBufferSize() };
	}

	// Create the blending setup
	{
		D3D12_BLEND_DESC& desc = psoDesc.BlendState;
		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	// Create the rasterizer state
	{
		D3D12_RASTERIZER_DESC& desc = psoDesc.RasterizerState;
		desc.FillMode = D3D12_FILL_MODE_SOLID;
		desc.CullMode = D3D12_CULL_MODE_NONE;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc.DepthClipEnable = true;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	// Create depth-stencil State
	{
		D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.StencilEnable = false;
		desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.BackFace = desc.FrontFace;
	}

	if (m_pD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState)) != S_OK)
		return false;

	CreateFontsTexture();

	return true;
}

void DXImContext::InvalidateDeviceObjects()
{
	if (!m_pD3dDevice)
		return;

	if (m_pVertexShaderBlob) { m_pVertexShaderBlob->Release(); m_pVertexShaderBlob = NULL; }
	if (m_pPixelShaderBlob) { m_pPixelShaderBlob->Release(); m_pPixelShaderBlob = NULL; }
	if (m_pRootSignature) { m_pRootSignature->Release(); m_pRootSignature = NULL; }
	if (m_pPipelineState) { m_pPipelineState->Release(); m_pPipelineState = NULL; }
	if (m_pFontTextureResource) { m_pFontTextureResource->Release(); m_pFontTextureResource = NULL; ImGui::GetIO().Fonts->TexID = NULL; } // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.

	for (UINT i = 0; i < m_uiNumFramesInFlight; ++i)
	{
		if (m_pFrameResources[i].IB) { m_pFrameResources[i].IB->Release(); m_pFrameResources[i].IB = NULL; }
		if (m_pFrameResources[i].VB) { m_pFrameResources[i].VB->Release(); m_pFrameResources[i].VB = NULL; }
	}
}

void DXImContext::Deinitialize()
{
	//// Make sure the GPU is done processing
	//if (m_pContext->GetDeviceResources())
	//	m_pContext->GetDeviceResources()->WaitForGpu();
	//
	InvalidateDeviceObjects();
}

void DXImContext::CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	uint32_t uiID = m_pContext->GetTextureManager()->CreateTexture(m_pContext->GetEngine("Direct"), "ImGui Font", pixels, UVector2(width, height));

	PTextureManager* pTextureManager = m_pContext->GetTextureManager();
	m_FontSrvCpuDescHandle = pTextureManager->GetHeapManager()->GetCPUHandle(uiID);
	m_FontSrvGpuDescHandle = pTextureManager->GetHeapManager()->GetGPUHandle(uiID);

	// Store our identifier
	static_assert(sizeof(ImTextureID) >= sizeof(m_FontSrvGpuDescHandle.ptr), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
	io.Fonts->TexID = (ImTextureID)m_FontSrvGpuDescHandle.ptr;
}