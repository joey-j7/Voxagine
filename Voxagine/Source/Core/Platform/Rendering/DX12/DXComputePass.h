#pragma once

#include "Core/Platform/Rendering/ComputePass.h"
#include "Core/Platform/Rendering/RenderDefines.h"

#include <wrl/client.h>

class DXHeapManager;

class DXComputePass : public ComputePass
{
	friend class Mapper;
	friend class View;

public:
	DXComputePass(PRenderContext* pContext) : ComputePass(pContext) {}
	~DXComputePass();

	virtual void Compute(PCommandEngine* pEngine) override;

	DXHeapManager* GetHeapManager() const { return m_pHeapManager; }
	
protected:
	void SetHeapManager(DXHeapManager* pManager) { m_pHeapManager = pManager; }

	virtual void Init(const ComputePass::Data& data) override;
	uint32_t GetViewID(const std::string& sName);
	
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;

	std::vector<uint32_t> m_uiHeapIDs; 
	DXHeapManager* m_pHeapManager = nullptr;
	bool m_bIsHeapOwner = false;

	// Lookup table
	std::unordered_map<std::string, uint32_t> m_mViewIDs;
};