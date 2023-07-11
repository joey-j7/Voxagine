#pragma once

#include <vector>
#include <stdint.h>

enum RenderDataType {
	VOXEL,
	VERTEX
};

struct RenderData {
	RenderData(void* pData, uint32_t uiTotalSize, std::vector<uint32_t> vStrides, RenderDataType type = VOXEL) {
		m_pData = pData;
		m_uiTotalSize = uiTotalSize;

		m_vStrides = vStrides;
		m_type = type;
	};

	void* m_pData;
	uint32_t m_uiTotalSize;

	std::vector<uint32_t> m_vStrides;
	RenderDataType m_type;
};