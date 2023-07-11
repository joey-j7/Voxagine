#pragma once

#include "Core/Platform/Rendering/CommandEngine.h"

#include <d3d12.h>
#include <wrl/client.h>

class DX12RenderContext;

class DXCommandEngine : public CommandEngine
{
	friend class RenderContext;
	friend class DX12RenderContext;
	friend class DXRenderPass;
	
public:
	DXCommandEngine(DX12RenderContext* pContext, Info info);
	~DXCommandEngine();

	virtual void Reset() override;
	virtual void Start() override;

	virtual void Execute() override;

	virtual void Wait(PCommandEngine* pEngine, uint64_t uiValue) override;

	ID3D12Fence* GetFence() const { return m_pFence.Get(); }
	ID3D12GraphicsCommandList* GetList() const { return m_pCommandList.Get(); }
	ID3D12CommandQueue* GetQueue() const { return m_pCommandQueue.Get(); }

	virtual void ApplyBarriers() override;
	virtual void WaitForGPU() override;

protected:
	virtual void Signal() override;
	virtual void AdvanceFrame() override;

	D3D12_COMMAND_LIST_TYPE MapType(Type type);

	DX12RenderContext* m_pContext = nullptr;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;

	std::vector<CD3DX12_RESOURCE_BARRIER> m_Barriers;

	HANDLE m_FenceEvent;
};