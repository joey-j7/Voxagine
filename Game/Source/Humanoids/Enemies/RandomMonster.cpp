#include "pch.h"
#include "RandomMonster.h"

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<RandomMonster>("RandomMonster")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

void RandomMonster::Start()
{
	// Functionality is in Spawner
	Destroy();
}
