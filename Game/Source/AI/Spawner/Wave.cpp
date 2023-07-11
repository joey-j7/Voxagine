#include "Wave.h"

#include <External/rttr/policy.h>

#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Wave>("Wave")
		.constructor</*std::vector<Wave::SpawnProperties>&&, */bool, float, float>()(
			rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr,

			rttr::default_arguments(true, 5.0f, 0.0f)
		)
		.property("Is Alive", &Wave::m_bIsAlive)(RTTR_PRIVATE)
		.property("Count Enemies", &Wave::m_iSizeOfEnemies)(RTTR_PUBLIC)
		.property("Burst", &Wave::bBurst)(RTTR_PUBLIC)
		.property("Next Spawn Timer", &Wave::fNextSpawnTimer)(RTTR_PUBLIC)
		.property("Radius", &Wave::m_fRadius)(RTTR_PUBLIC)
		.property("Minimal Particle Force", &Wave::v3MinForce)(RTTR_PUBLIC)
		.property("Maximal Particle Force", &Wave::v3MaxForce)(RTTR_PUBLIC)
		// .property("Spawn Properties", &Wave::m_lSpawnProperties)
		.property("Is Started", &Wave::HasStarted, &Wave::IsStarted)(RTTR_PRIVATE);
}

Wave::Wave(/* std::vector<SpawnProperties>&& lSpawnProperties, */bool bBurst, float fRadius, float fNextSpawnTimer) 
	// : m_lSpawnProperties(lSpawnProperties)
{
	this->bBurst = bBurst;
	this->fNextSpawnTimer = fNextSpawnTimer;
	m_fRadius = fRadius;
	// m_iSizeOfEnemies = m_lSpawnProperties.size();
}

void Wave::Clear() 
{
	// TODO maybe remove from the group
	// delete pFlock;

	m_bIsAlive = false;
}