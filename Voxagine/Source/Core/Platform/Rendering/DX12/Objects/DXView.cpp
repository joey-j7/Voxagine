#include "pch.h"

#include "Core/Platform/Rendering/Objects/View.h"
#include "Core/Platform/Rendering/DX12/DX12RenderContext.h"
#include "Core/Platform/Rendering/DX12/DXHelper.h"
#include "Core/Platform/Rendering/DX12/DXCommandEngine.h"
#include "Core/Platform/Rendering/DX12/DXRenderPass.h"

#include "Core/Platform/Rendering/DX12/Managers/DXHeapManager.h"

#include <xlocbuf>
#include <codecvt>

#include <d3d12.h>

View::~View()
{
	// TODO: Remove SRV handle
	if (m_pNativeTexture)
		m_pNativeTexture->Release();
}

PResourceStates View::SetState(PCommandEngine* pEngine, PResourceStates state)
{
	PResourceStates oldState = m_Info.m_State;
	m_Info.m_State = state;

	return oldState;
}


void View::Resize(const UVector3& uSize)
{
	// Reset pointer and make new object
	if (m_pNativeTexture)
		m_pNativeTexture->Release();

	ID3D12Device* pDevice = m_pContext->GetDevice();
	m_Info.m_Size = uSize;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC desc;

	if (m_Info.m_DimensionType == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		desc = CD3DX12_RESOURCE_DESC::Tex2D(
			static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat),
			static_cast<UINT64>(m_Info.m_Size.x), static_cast<UINT>(m_Info.m_Size.y),
			1, 1, 1, 0,
			m_Info.m_Type == E_DEPTH_STENCIL_VIEW ?
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL :
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		);
	}
	else if (m_Info.m_DimensionType == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		desc = CD3DX12_RESOURCE_DESC::Tex3D(
			static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat),
			static_cast<UINT64>(m_Info.m_Size.x), static_cast<UINT>(m_Info.m_Size.y),
			static_cast<UINT16>(m_Info.m_Size.z), 1,
			m_Info.m_Type == E_DEPTH_STENCIL_VIEW ?
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL :
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		);
	}
	else
	{
		// Not supported
		assert(false);
	}

	D3D12_CLEAR_VALUE clearValue = {};

	if (m_Info.m_Type == E_DEPTH_STENCIL_VIEW)
	{
		clearValue.Format = static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat);
		clearValue.Color[0] = FLT_MAX;
		clearValue.Color[1] = 0.f;
		clearValue.Color[2] = 0.f;
		clearValue.Color[3] = 0.f;

		clearValue.DepthStencil.Depth = 1.0;
		clearValue.DepthStencil.Stencil = 0;
	}
	else
	{
		if (m_Info.m_ColorFormat == E_R32_FLOAT)
			clearValue = { static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat), { FLT_MAX } };
		else
			clearValue = { static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat), { 0.f, 0.f, 0.f, 0.f } };
	}

	ThrowIfFailed(
		pDevice->CreateCommittedResource(
			&heapProperties,
			(m_Info.m_Type == E_DEPTH_STENCIL_VIEW) ? D3D12_HEAP_FLAG_NONE : D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			&desc,
			static_cast<D3D12_RESOURCE_STATES>(m_Info.m_State), &clearValue,
			IID_PPV_ARGS(&m_pNativeTexture)
		)
	);

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring nameW = converter.from_bytes(m_Info.m_Name);
	m_pNativeTexture->SetName(nameW.c_str());
	
	for (TargetData& data : m_TargetData)
	{
		CreateTarget(data);
	}
}

void View::AddTarget(PRenderPass* pRenderPass, uint32_t uiID, Type type)
{
	bool bFound = false;
	TargetData data = { pRenderPass, uiID, type };

	for (TargetData& found : m_TargetData)
	{
		if (found.m_pTarget == pRenderPass)
		{
			bFound = true;
			break;
		}
	}

	if (!bFound)
		m_TargetData.push_back({ pRenderPass, uiID, type });

	CreateTarget(data);
}

void View::CreateTarget(TargetData& data)
{
	// Create View
	switch (data.m_Type)
	{
	case E_SHADER_RESOURCE_VIEW:
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		switch (m_Info.m_DimensionType)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = 1;
			break;
		}
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MipLevels = 1;
			break;
		}
		default:
		{
			assert(false);
			break;
		}};

		desc.Format = static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat);
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		m_pContext->GetDevice()->CreateShaderResourceView(m_pNativeTexture, &desc, data.m_pTarget->GetHeapManager()->GetCPUHandle(data.m_uiID));
		break;
	}
	case E_DEPTH_STENCIL_VIEW:
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.Format = static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat);
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Flags = D3D12_DSV_FLAG_NONE;
		desc.Texture2D.MipSlice = 0;

		m_pContext->GetDevice()->CreateDepthStencilView(m_pNativeTexture, &desc, data.m_pTarget->GetDSVCPUHandle(data.m_uiID));
		break;
	}
	case E_RENDER_TARGET_VIEW:
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = static_cast<DXGI_FORMAT>(m_Info.m_ColorFormat);
		desc.Texture2D.MipSlice = 0;

		m_pContext->GetDevice()->CreateRenderTargetView(m_pNativeTexture, &desc, data.m_pTarget->GetRTVCPUHandle(data.m_uiID));
		break;
	}
	default:
	{
		assert(false);
		break;
	}
	}
}