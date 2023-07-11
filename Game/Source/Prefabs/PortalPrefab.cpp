#include "PortalPrefab.h"

#include <External/rttr/registration.h>

#include "Core/ECS/World.h"
#include "Core/Application.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Components/VoxAnimator.h"
#include "Core/ECS/Systems/Rendering/DebugRenderer.h"

#include "AI/Spawner/Spawner.h"

#include "General/FlashBehavior.h"


RTTR_REGISTRATION
{
	rttr::registration::class_<PortalPrefab>("PortalPrefab")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

PortalPrefab::PortalPrefab(World* world) : Entity(world)
{
	const std::string sName = "Portal" + std::to_string(GetId());
	SetName(sName);

	AddComponent<VoxRenderer>();
	auto pPortalAnimator = AddComponent<VoxAnimator>();

	// VoxModel* pAimModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Portals/Portal1.vox");
	// pPortalRenderer->SetModel(pAimModel);
	// pPortalRenderer->SetRotationAngleLimit(0);

	pPortalAnimator->SetAnimModelFilePath("Content/Models/Portals/Portal.anim.vox");

	std::vector<std::string> m_Models = { "Content/Models/Portals/Portal.anim.vox" };
	pPortalAnimator->SetAnimationFiles(m_Models);
}

void PortalPrefab::Awake()
{
	AddTag("Portal");

	m_pSpawnComponent = AddComponent<Spawner>();
	if(!m_pSpawnComponent)
		m_pSpawnComponent = GetComponent<Spawner>();

	// FlashComponent
	const auto pFlashingComponent = GetComponent<FlashBehavior>();
	if (!pFlashingComponent)
		 AddComponent<FlashBehavior>();

	auto pCollider = AddComponent<BoxCollider>();
	if (!pCollider)
		pCollider = GetComponent<BoxCollider>();

	pCollider->SetTrigger(true);

	pCollider->AutoFit();

	SetPersistent(true);
}

void PortalPrefab::OnDrawGizmos(float)
{
	if(m_pSpawnComponent)
	{
		float x = GetTransform()->GetPosition().x;
		float z = GetTransform()->GetPosition().z;

		const Vector3 location = { x, GetTransform()->GetPosition().y, z };

		if (auto debugRenderer = GetWorld()->GetDebugRenderer())
		{
			DebugSphere sphere;

			sphere.m_Center = location;
			sphere.m_fRadius = m_pSpawnComponent->fRadius;
			sphere.m_Color = VColors::Purple;

			debugRenderer->AddSphere(sphere);
		}
	}
}
