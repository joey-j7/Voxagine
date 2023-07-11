#include "KeyPrefab.h"

#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "AI/Spawner/SpawnerEntity.h"


#include "External/rttr/registration.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<KeyPrefab>("KeyPrefab")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

KeyPrefab::KeyPrefab(World* world) : Entity(world)
{
	VoxModel* pVoxModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Props/key.vox");
	auto VoxRenderComp = AddComponent<VoxRenderer>();
	auto pBoxCollider = AddComponent<BoxCollider>();
	VoxRenderComp->SetModel(pVoxModel);
	pBoxCollider->SetBoxSize(pVoxModel->GetFrame(0));
	pBoxCollider->SetTrigger(true);
}

void KeyPrefab::SetTriggerMethod(std::function<bool()> triggerMethod)
{
	m_TriggerMethod = triggerMethod;
}

void KeyPrefab::OnCollisionEnter(Collider* pCollider, const Manifold&)
{
	if (pCollider->GetOwner()->GetName() == "Player") {
		if (m_TriggerMethod && m_TriggerMethod())
		{ // If we executed our code destroy our self.
			Destroy();
		}
	}
}
