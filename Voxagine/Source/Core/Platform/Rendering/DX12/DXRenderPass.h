#pragma once

#include "Core/Platform/Rendering/RenderPass.h"
#include "Core/Platform/Rendering/RenderDefines.h"

#include <wrl/client.h>

class DXHeapManager;

class DXRenderPass : public RenderPass
{
	friend class Mapper;
	friend class View;
	friend class DXRenderPass;

public:
	DXRenderPass(PRenderContext* pContext) : RenderPass(pContext) {}
	~DXRenderPass();

	virtual void Begin(PCommandEngine* pEngine) override;
	virtual void Draw(PCommandEngine* pEngine) override;
	virtual void End(PCommandEngine* pEngine) override;

	virtual void Clear(PCommandEngine* pEngine) override;

	virtual View* GetTargetView(uint32_t i = 0) const override;
	virtual View* GetDepthView() const override;

	virtual void Resize(UVector2 uSize) override;

	DXHeapManager* GetHeapManager() const { return m_pHeapManager; }
	virtual UVector2 GetTargetSize() const override;

protected:
	void SetHeapManager(DXHeapManager* pManager) { m_pHeapManager = pManager; }

	virtual void Init(const RenderPass::Data& data) override;
	uint32_t GetViewID(const std::string& sName);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(uint32_t uiID);
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle(uint32_t uiID);

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetRTVGPUHandle(uint32_t uiID);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetDSVGPUHandle(uint32_t uiID);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;

	std::vector<uint32_t> m_uiHeapIDs;
	DXHeapManager* m_pHeapManager = nullptr;
	bool m_bIsHeapOwner = false;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRTVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;

	uint32_t m_uiRTVDescriptorSize = 0;
	uint32_t m_uiDSVDescriptorSize = 0;

	// Lookup table
	std::unordered_map<std::string, uint32_t> m_mViewIDs;
};