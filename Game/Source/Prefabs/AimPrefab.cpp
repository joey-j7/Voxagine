#include "AimPrefab.h"

#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Components/BoxCollider.h"

#include "External/rttr/registration.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<AimPrefab>("AimPrefab")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

AimPrefab::AimPrefab(World* world) : Entity(world)
{
	SetName("Aim");

	pAimRenderer = AddComponent<VoxRenderer>();

	VoxModel* pAimModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Character_Models/Player/Aim.vox");
	pAimRenderer->SetModel(pAimModel);
	pAimRenderer->SetRotationAngleLimit(0);
}

void AimPrefab::Start()
{
	Entity::Start();

	GetTransform()->SetPosition({0.f, 0.f, 20.f}, true);
}
