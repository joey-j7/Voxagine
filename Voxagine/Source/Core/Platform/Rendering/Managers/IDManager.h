#pragma once

#include "Core/Platform/Rendering/RenderDefines.h"

#include <stdint.h>

class IDManager
{
public:
	IDManager(PRenderContext* pContext, uint32_t uiMaxCount);

	uint32_t GetMaxCount() const { return m_uiMaxCount; }

protected:
	void FreeID(uint32_t uiID);
	uint32_t ReserveID();

	PRenderContext* m_pContext = nullptr;
	uint32_t m_uiMaxCount = 0;

private:
	std::vector<bool> m_bAvailableSlots;
};