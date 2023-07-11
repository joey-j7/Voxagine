#include "pch.h"
#include "HordeMonster.h"

#include <External/rttr/registration.h>
#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/VoxAnimator.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<HordeMonster>("HordeMonster")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

HordeMonster::HordeMonster(World* world) :
	Monster(world)
{}

void HordeMonster::Awake()
{
	Monster::Awake();
	SetName("HordeMonster");
}
