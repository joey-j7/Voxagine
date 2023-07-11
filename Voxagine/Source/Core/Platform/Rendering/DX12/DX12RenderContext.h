#pragma once

#include "Core/Platform/Rendering/RenderContext.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <dxgi1_6.h>

class DX12RenderContext : public RenderContext {
public:
	DX12RenderContext(Platform* pPlatform);
	virtual ~DX12RenderContext();

	virtual void Initialize() override;
	virtual void Deinitialize() override;

	virtual void Clear() override;
	virtual bool Present() override;

	virtual bool OnResize(uint32_t uiWidth, uint32_t uiHeight) override;

	ID3D12Device* GetDevice() const { return m_pDevice.Get(); }
	IDXGISwapChain3* GetSwapChain() const { return m_pSwapChain.Get(); }

private:
	virtual void LoadShader(ShaderReference* pTextureReference) override {};
	virtual void DestroyShader(const ShaderReference* textureRef) override {};

	void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);

	// Adapter info
	bool m_bUseWarpDevice = false;

	// Pipeline objects
	CD3DX12_VIEWPORT m_Viewport;
	CD3DX12_RECT m_ScissorRect;

	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_pSwapChain;
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_pDXGIFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenRenderPasses[m_uiFrameCount];
};