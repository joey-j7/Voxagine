#pragma once

#include "ImContext.h"
#include <memory>

#include <d3d12.h>
#include <wrl/client.h>

class DX12RenderContext;

struct VERTEX_CONSTANT_BUFFER
{
	float mvp[4][4];
};

struct FrameResources
{
	ID3D12Resource* IB;
	ID3D12Resource* VB;
	int VertexBufferSize;
	int IndexBufferSize;
};

class DXImContext : public ImContext {
public:
	DXImContext(DX12RenderContext* pContext);

	virtual void NewFrame() override;

	virtual void Draw(ImDrawData* drawData, ID3D12GraphicsCommandList* pCommandList);

	bool CreateDeviceObjects();
	void InvalidateDeviceObjects();

	virtual void Deinitialize() override;

protected:
	virtual void Draw(ImDrawData* drawData) override;
	void CreateFontsTexture();

	// DirectX data
	static ID3D12Device* m_pD3dDevice;
	static ID3D10Blob* m_pVertexShaderBlob;
	static ID3D10Blob*                  m_pPixelShaderBlob;
	static ID3D12RootSignature*         m_pRootSignature;
	static ID3D12PipelineState*         m_pPipelineState;
	static DXGI_FORMAT                  m_RTVFormat;
	static ID3D12Resource*              m_pFontTextureResource;
	static D3D12_CPU_DESCRIPTOR_HANDLE  m_FontSrvCpuDescHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE  m_FontSrvGpuDescHandle;

	static FrameResources*              m_pFrameResources;
	static UINT                         m_uiNumFramesInFlight;
	static UINT                         m_uiFrameIndex;

	ID3D12GraphicsCommandList* m_pCommandList = nullptr;
	DX12RenderContext* m_pContext;
};