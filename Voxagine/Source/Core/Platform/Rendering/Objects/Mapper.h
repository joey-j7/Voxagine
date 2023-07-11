#pragma once

#include "Core/Platform/Rendering/RenderDefines.h"
#include "Core/Math.h"

#include <string>

class RenderPass;
class View;

class Mapper
{
public:
	struct Info
	{
		GPUAccessType				m_GPUAccessType = E_READ_ONLY;

		uint32_t					m_uiElementSize = 0;
		uint32_t					m_uiElementCount = 0;

		bool						m_bHasBackBuffer = false;

		PResourceFormat				m_ColorFormat = R_DEF_RESOURCE_FORMAT;

		std::string					m_Name = "Unnamed";
	};

	Mapper(PRenderContext* pContext, const Info& info, bool bCreate = true);
	~Mapper();

	Event<uint32_t*&> BufferSwapped;

	const Info& GetInfo() const { return m_Info; }
	bool Resize(uint32_t uiElementCount, uint32_t uiElementSize);

	void Map();
	void Unmap();

	inline uint32_t*& GetData() { return m_pData[m_uiCurrentBackBuffer]; }
	inline uint32_t*& GetBackBufferData() { return m_pData[(m_uiCurrentBackBuffer + 1) % 2]; }

	uint32_t GetBackBufferIndex() const { return m_uiCurrentBackBuffer; }

	void SwapBuffer();

#ifdef _ORBIS
	PUploadBuffer*& GetNative() { return m_pNative; }
#else
	PResource* GetNative() { return m_pMapper[m_uiCurrentBackBuffer].Get(); }
#endif

	void AddTarget(PComputePass* pComputePass, uint32_t uiID);
	void AddTarget(PRenderPass* pRenderPass, uint32_t uiID);

protected:
	void CreateView(PComputePass* pComputePass, uint32_t uiID);
	void CreateView(PRenderPass* pRenderPass, uint32_t uiID);

	void CreateView(uint64_t uiGPUAddress, PResource* pMapper);

	PRenderContext* m_pContext = nullptr;

	Info m_Info;

	bool m_bIsMapped = false;
	uint32_t m_uiCurrentBackBuffer = 0;

	std::unordered_map<PRenderPass*, uint32_t> m_mRenderPasses;
	std::unordered_map<PComputePass*, uint32_t> m_mComputePasses;

	uint32_t* m_pData[2] = { nullptr, nullptr };

#ifdef _ORBIS
	PUploadBuffer* m_pNative = nullptr;
#endif

	R_PTR_TYPE(PResource) m_pMapper[2];
};