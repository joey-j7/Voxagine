#include "pch.h"
#include "ChunkViewer.h"

#include <rttr/registration>
#include <rttr/policy.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<ChunkViewer>("ChunkViewer")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr);
}

ChunkViewer::ChunkViewer(Entity* pOwner):
	Component(pOwner)
{
}
