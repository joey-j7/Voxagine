#pragma once

#include <stdint.h>

#include "Core/Math.h"
#include "RenderDefines.h"

#include "RenderPassInc.h"
#include "ComputePassInc.h"

class Buffer;
class Mapper;
class View;

class CommandEngine
{
	friend class RenderContext;

	// TODO: remove these
	friend class DX12RenderContext;
	friend class ORBRenderContext;
	friend class GLRenderContext;

public:
	enum Type
	{
		E_DIRECT,
		E_COPY,
		E_COMPUTE
	};

	struct Info
	{
		Type			m_Type = E_DIRECT;
		std::string		m_Name = "Unnamed";
	};

	CommandEngine(const Info& info) { m_Info = info; }
	virtual ~CommandEngine() {}

	// Reset allocator
	virtual void Reset() = 0;

	// Reset command List
	virtual void Start() = 0;

	// Begin, draw and end render target
	void Begin(PRenderPass* pRenderPass)
	{
		pRenderPass->Begin(Get());
	}

	void Compute(PComputePass* pComputePass)
	{
		pComputePass->Compute(Get());
	}

	void Draw(PRenderPass* pRenderPass)
	{
		pRenderPass->Draw(Get());
	}

	void End(PRenderPass* pRenderPass)
	{
		pRenderPass->End(Get());
	}

	PCommandEngine* Get();

	// Set texture so a certain state
	void Set(View* pView, PResourceStates state)
	{
		pView->SetState(Get(), state);
	}

	// Close command list and execute
	virtual void Execute() = 0;

	// Wait until fence has reached value
	virtual void Wait(PCommandEngine* pEngine, uint64_t uiValue) = 0;

	virtual void ApplyBarriers() = 0;

	bool IsStarted() const { return m_bIsStarted; }

	// Get current fence value
	uint64_t GetValue() { return m_uiFenceValue; }

	const Info& GetInfo() const { return m_Info; }

	// Wait until GPU has finished executing
	virtual void WaitForGPU() = 0;

protected:
	// Signal a value to fence
	virtual void Signal()
	{
		m_uiFenceValue++;
	}

	// Prepare to render next frame
	virtual void AdvanceFrame() = 0;

	bool m_bIsStarted = false;

	Info m_Info;
	uint64_t m_uiFenceValue = 0;
};