#pragma once

#include <vector>
#include <stdint.h>

class RenderBuffer {
public:
	virtual std::vector<uint32_t> GetStrides() const { return {}; };
	virtual uint32_t GetTotalSize() const { return 0; };
};