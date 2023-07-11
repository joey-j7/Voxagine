#include "HealthUI.h"
#include "Core/ECS/World.h"
#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include <Core/ECS/Components/SpriteRenderer.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<HealthUI>("HealthUI")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

//Constructor
HealthUI::HealthUI(World* world) : Entity(world)
{
	//Sets the default entities name
	SetName("HealthUI");
}

void HealthUI::Awake()
{
	AddComponents();
}

void HealthUI::AddComponents()
{
	// Add Vox Renderer component to the entity
	m_pSpriteRenderer = GetComponent<SpriteRenderer>();
	if (!m_pSpriteRenderer)
		m_pSpriteRenderer = AddComponent<SpriteRenderer>();
	
	m_pSpriteRenderer->SetFilePath(healthSprite);
	m_pSpriteRenderer->SetAlignment(RA_CENTERED);
	m_pSpriteRenderer->SetScreenAlignment(RA_CENTERED);
	m_pSpriteRenderer->SetToScreenSpace(TRUE);
	m_pSpriteRenderer->SetScaleWithScreen(TRUE);
}

void HealthUI::SetHealthCullingEnd(float currentHealth)
{
	m_pSpriteRenderer->SetCullingEnd(Vector2(currentHealth, 1.f));
}