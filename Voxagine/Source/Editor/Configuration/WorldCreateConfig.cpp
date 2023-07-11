#include "pch.h"
#include "WorldCreateConfig.h"

#include <Core/MetaData/PropertyTypeMetaData.h>
#include <External/rttr/registration>
#include <External/rttr/policy.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<WorldCreateConfig>("WorldCreateConfig")
		.constructor<>()(rttr::policy::ctor::as_object)
		.property("WorldChunkSize", &WorldCreateConfig::GetWorldChunkSize, &WorldCreateConfig::SetWorldChunkSize) (RTTR_PUBLIC)
		.property("MaxParticles", &WorldCreateConfig::GetMaxParticles, &WorldCreateConfig::SetMaxParticles) (RTTR_PUBLIC);
}

void WorldCreateConfig::SetWorldChunkSize(uint32_t size)
{
	m_uiChunkWorldSize = size;

	if (size == 2)
		m_uiChunkWorldSize = 3;

	if (size == 0)
		m_uiChunkWorldSize = 1;
}
