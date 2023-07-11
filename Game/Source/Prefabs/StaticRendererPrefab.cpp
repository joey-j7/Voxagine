#include "StaticRendererPrefab.h"

#include "External/rttr/registration.h"

#include "Core/ECS/World.h"

#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Components/BoxCollider.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<StaticRendererPrefab>("Static Renderer")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

StaticRendererPrefab::StaticRendererPrefab(World* world) : Entity(world)
{

}

void StaticRendererPrefab::Awake()
{
	Entity::Awake();

	if(GetName() == "Entity" + std::to_string(GetId())) // Override only if it has a basic name
		SetName("Static Renderer");

	SetStatic(true);

	VoxRenderer* pRenderer = GetComponent<VoxRenderer>();
	if (!pRenderer) AddComponent<VoxRenderer>();

	BoxCollider* pCollider = GetComponent<BoxCollider>();
	if (!pCollider) AddComponent<BoxCollider>();
}
