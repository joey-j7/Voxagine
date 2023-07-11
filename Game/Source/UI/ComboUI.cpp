#include "ComboUI.h"
#include "Core/ECS/World.h"
#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include <Core/ECS/Components/SpriteRenderer.h>
#include "Core/ECS/Components/TextRenderer.h"
#include "Core/MetaData/PropertyTypeMetaData.h"

#include <string>

RTTR_REGISTRATION
{
	rttr::registration::class_<ComboUI>("ComboUI")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

//Constructor
ComboUI::ComboUI(World* world) : Entity(world)
{
	//Sets the default entities name
	SetName("ComboUI");
}

void ComboUI::Awake()
{
	AddComponents();
}

void ComboUI::AddComponents()
{
	// Add Vox Renderer component to the entity
	m_pTextRenderer = GetComponent<TextRenderer>();
	if (!m_pTextRenderer)
		m_pTextRenderer = AddComponent<TextRenderer>();

	m_pTextRenderer->SetText("Combo: 0");
	m_pTextRenderer->SetAlignment(RA_CENTERED);
	m_pTextRenderer->SetScreenAlignment(RA_CENTERED);
	m_pTextRenderer->SetScaleWithScreen(TRUE);
	
}

void ComboUI::SetComboUI(std::string currentCombo)
{
	m_pTextRenderer->SetText("Combo: " + currentCombo);
}
