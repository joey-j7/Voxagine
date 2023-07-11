#pragma once

#include "External/imgui/imgui.h"

class ImContext {
public:
	virtual ~ImContext() = default;

	virtual void NewFrame() = 0;
	virtual void Draw(ImDrawData* pDrawData) = 0;

	virtual void Deinitialize() = 0;
};