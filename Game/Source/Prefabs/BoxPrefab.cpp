#include "BoxPrefab.h"

#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Components/BoxCollider.h"

#include "External/rttr/registration.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<BoxPrefab>("BoxPrefab")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

BoxPrefab::BoxPrefab(World* world) : Entity(world)
{
	VoxModel* pVoxModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Props/15x15x15.vox");
	auto VoxRenderComp = AddComponent<VoxRenderer>();
	auto pBoxCollider = AddComponent<BoxCollider>();
	SetStatic(true);
	VoxRenderComp->SetModel(pVoxModel);
	pBoxCollider->SetBoxSize(pVoxModel->GetFrame(0));
	pBoxCollider->SetLayer(CollisionLayer::CL_OBSTACLES);
}

void BoxPrefab::Start()
{
	Entity::Start();
}
