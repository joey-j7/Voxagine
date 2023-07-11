#include "pch.h"
#include "DXCommandEngine.h"

#include "Core/Platform/Rendering/DX12/DX12RenderContext.h"
#include "Core/Platform/Rendering/DX12/DXRenderPass.h"
#include "Core/Platform/Rendering/DX12/DXHelper.h"
#include "Core/Platform/Rendering/Objects/Mapper.h"

#include <xlocbuf>
#include <codecvt>

DXCommandEngine::DXCommandEngine(DX12RenderContext* pContext, Info info) : CommandEngine(info)
{
	D3D12_COMMAND_LIST_TYPE listType = MapType(m_Info.m_Type);

	m_pContext = pContext;

	ID3D12Device* pDevice = m_pContext->GetDevice();

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = listType;

	ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)));
	ThrowIfFailed(pDevice->CreateCommandAllocator(listType, IID_PPV_ARGS(&m_pCommandAllocator)));

	// Create the command list.
	ThrowIfFailed(pDevice->CreateCommandList(0, listType, m_pCommandAllocator.Get(), NULL, IID_PPV_ARGS(&m_pCommandList)));
	
	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(m_pCommandList->Close());

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));

		// Create an event handle to use for frame synchronization.
		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_FenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		AdvanceFrame();
	}

	// Set names
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring nameW = converter.from_bytes(m_Info.m_Name);
	m_pCommandQueue->SetName(nameW.c_str());
	m_pCommandAllocator->SetName(nameW.c_str());
	m_pCommandList->SetName(nameW.c_str());
	m_pFence->SetName(nameW.c_str());
}

DXCommandEngine::~DXCommandEngine()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	Signal();
	WaitForGPU();

	CloseHandle(m_FenceEvent);
}

void DXCommandEngine::Reset()
{
	WaitForGPU();

	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(m_pCommandAllocator->Reset());
}

void DXCommandEngine::Start()
{
	if (m_bIsStarted) return;
	m_bIsStarted = true;

	// However, when Execute() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), NULL));
}

void DXCommandEngine::Execute()
{
	// Close command list
	ThrowIfFailed(m_pCommandList->Close());

	m_bIsStarted = false;

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Signal
	Signal();
}

void DXCommandEngine::Wait(PCommandEngine* pEngine, uint64_t uiValue)
{
	DXCommandEngine* pDXEngine = static_cast<DXCommandEngine*>(pEngine);

	// Schedule a Signal command in the queue.
	ThrowIfFailed(
		m_pCommandQueue->Wait(
			pDXEngine->GetFence(),
			uiValue
		)
	);
}

void DXCommandEngine::ApplyBarriers()
{
	if (m_Barriers.empty())
		return;

	m_pCommandList->ResourceBarrier(static_cast<uint32_t>(m_Barriers.size()), m_Barriers.data());
	m_Barriers.clear();
}

void DXCommandEngine::Signal()
{
	// Increments fence value
	CommandEngine::Signal();

	// Signals new max fence value
	ThrowIfFailed(m_pCommandQueue->Signal(m_pFence.Get(), m_uiFenceValue));
}

void DXCommandEngine::WaitForGPU()
{
	// Wait until the fence has been processed.
	ThrowIfFailed(m_pFence->SetEventOnCompletion(m_uiFenceValue, m_FenceEvent));
	WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
}

void DXCommandEngine::AdvanceFrame()
{
	// If the next frame is not ready to be rendered yet, wait until it is ready.
	WaitForGPU();

	// Set the fence value for the next frame.
	m_uiFenceValue = 0;

	// Make sure the fence starts at signal 0
	m_pFence->Signal(0);
}

D3D12_COMMAND_LIST_TYPE DXCommandEngine::MapType(Type type)
{
	switch (type)
	{
	case CommandEngine::E_COMPUTE:	return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case CommandEngine::E_COPY:		return D3D12_COMMAND_LIST_TYPE_COPY;
	default:						return D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
}
