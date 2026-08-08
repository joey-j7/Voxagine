#pragma once

#include "Core/Platform/Rendering/RenderDefines.h"

/* Was almost entirely PS4: Gnm contexts, Garlic/Onion allocators and a window
   handle, all behind _ORBIS. That target is gone, and what remains is the one
   field the rest of the engine actually reads. */
struct PlatformData
{
	PRenderPass* m_pScreenRenderPass = nullptr;
};
