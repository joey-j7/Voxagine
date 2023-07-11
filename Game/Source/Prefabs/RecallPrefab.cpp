#include "RecallPrefab.h"

#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/ECS/Components/SpriteRenderer.h"

#include "External/rttr/registration.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<RecallPrefab>("RecallPrefab")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		);
}

RecallPrefab::RecallPrefab(World* world) : Entity(world)
{
	SetName("Recall");

	pArrowRenderer = AddComponent<SpriteRenderer>();
	pArrowRenderer->SetFilePath("Content/UI_Art/Recall_Sprites/Recall_Icon.png");
	pArrowRenderer->SetColor(VColor(static_cast<unsigned char>(255u), 0u, 26u, 255u));
}

void RecallPrefab::Start()
{
	Entity::Start();

	GetTransform()->SetScale({ 0.02f, 0.02f, 1.0f }, true);
	GetTransform()->SetPosition({ 1.0f, 10.f, 0.f }, true);

	pArrowRenderer->SetBillboard(true);
}
