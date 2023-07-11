#include "pch.h"
#include "IDManager.h"


IDManager::IDManager(PRenderContext* pContext, uint32_t uiMaxCount)
{
	m_pContext = pContext;
	m_uiMaxCount = uiMaxCount;

	m_bAvailableSlots.resize(uiMaxCount);

	// Reset slots
	std::fill(m_bAvailableSlots.begin(), m_bAvailableSlots.end(), true);
}

void IDManager::FreeID(uint32_t uiID)
{
	m_bAvailableSlots[uiID] = true;
}

uint32_t IDManager::ReserveID()
{
	uint32_t uiID = UINT_MAX;

	for (uint32_t i = 0; i < m_uiMaxCount; ++i)
	{
		if (m_bAvailableSlots[i])
		{
			m_bAvailableSlots[i] = false;
			uiID = i;

			break;
		}
	}

	assert(uiID < m_uiMaxCount);
	return uiID;
}
