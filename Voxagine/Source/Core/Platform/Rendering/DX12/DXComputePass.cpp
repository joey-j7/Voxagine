#include "pch.h"

#include "Core/Platform/Rendering/DX12/DXComputePass.h"
#include "Core/Platform/Rendering/ComputePass.h"
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

void DXComputePass::Init(const ComputePass::Data& data)
{
	m_Data = data;

	ID3D12Device* pDevice = m_pContext->GetDevice();

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring nameW = converter.from_bytes(m_Data.m_Name);

	// Only when textures resources are bound
	size_t parameterCount = m_Data.m_Buffers.size() + m_Data.m_Textures.size() + m_Data.m_Mappers.size() + m_Data.m_uiBindlessResourceCount;

	uint32_t uiHeapCount = static_cast<uint32_t>(m_Data.m_Textures.size() + m_Data.m_Mappers.size() + m_Data.m_uiBindlessResourceCount);
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

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		// Describe and create the compute pipeline state object (PSO).
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.CS = CD3DX12_SHADER_BYTECODE(m_Data.m_pShader->GetNative());

		ThrowIfFailed(pDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState)));
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
	/*{
		for (View* pTexture : m_Data.m_Textures)
		{
			pTexture->AddTarget(this, m_uiHeapIDs[heapID], View::E_SHADER_RESOURCE_VIEW);
			heapID++;
		}
	}*/
}

DXComputePass::~DXComputePass()
{
	for (uint32_t uiID : m_uiHeapIDs)
	{
		m_pHeapManager->FreeResource(uiID);
	}

	if (m_bIsHeapOwner)
		delete m_pHeapManager;
}

void DXComputePass::Compute(PCommandEngine* pEngine)
{
	ID3D12GraphicsCommandList* pCommandList = pEngine->GetList();

	// Set pipeline state
	pCommandList->SetPipelineState(m_pPipelineState.Get());
	pCommandList->SetComputeRootSignature(m_pRootSignature.Get());

	if (m_pHeapManager)
	{
		// Set descriptor heaps
		ID3D12DescriptorHeap* heaps[] = { m_pHeapManager->GetNative() };
		pCommandList->SetDescriptorHeaps(1, heaps);
	}

	// Setup the texture and its sampler on the PS stage
	for (Buffer* pBuffer : m_Data.m_Buffers)
	{
		uint32_t uiViewID = GetViewID(pBuffer->GetInfo().m_Name);
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = pBuffer->GetGPUAddress();

		if (pBuffer->GetInfo().m_GPUAccessType == E_READ_WRITE)
		{
			pCommandList->SetComputeRootUnorderedAccessView(uiViewID, gpuAddress);
		}
		else
		{
			switch (pBuffer->GetInfo().m_Type)
			{
			case Buffer::E_CONSTANT:	pCommandList->SetComputeRootConstantBufferView(uiViewID, gpuAddress);	break;
			case Buffer::E_STRUCTURED:	pCommandList->SetComputeRootShaderResourceView(uiViewID, gpuAddress);	break;
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
			pCommandList->SetComputeRootDescriptorTable(viewID, m_pHeapManager->GetGPUHandle(m_uiHeapIDs[heapID + pMapper->GetBackBufferIndex()]));
		}
		else if (pMapper->GetInfo().m_GPUAccessType == E_READ_WRITE)
		{
			pCommandList->SetComputeRootUnorderedAccessView(viewID, pMapper->GetNative()->GetGPUVirtualAddress());
		}
		else
		{
			pCommandList->SetComputeRootShaderResourceView(viewID, pMapper->GetNative()->GetGPUVirtualAddress());
		}

		heapID += pMapper->GetInfo().m_bHasBackBuffer ? 2 : 1;
	}

	for (View* pTexture : m_Data.m_Textures)
	{
		uint32_t viewID = GetViewID(pTexture->GetInfo().m_Name);
		pCommandList->SetComputeRootDescriptorTable(viewID, m_pHeapManager->GetGPUHandle(m_uiHeapIDs[heapID]));

		heapID++;
	}

	for (uint32_t i = 0; i < m_Data.m_uiBindlessResourceCount; ++i)
	{
		uint32_t viewID = GetViewID(("Unbounded Resource " + std::to_string(i)).c_str());
		pCommandList->SetComputeRootDescriptorTable(viewID, m_pHeapManager->GetGPUHandle(0));

		heapID++;
	}

	pCommandList->Dispatch(m_Data.m_ThreadGroup.x, m_Data.m_ThreadGroup.y, m_Data.m_ThreadGroup.z);
}

uint32_t DXComputePass::GetViewID(const std::string & sName)
{
	auto find = m_mViewIDs.find(sName);
	if (find == m_mViewIDs.end()) {
		uint32_t uiSize = static_cast<uint32_t>(m_mViewIDs.size());
		auto descriptor = m_mViewIDs.emplace(sName, uiSize);
		return uiSize;
	}

	return find->second;
}