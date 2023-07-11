#include "ComboIcon.h"
#include "Core/ECS/World.h"
#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include <Core/ECS/Components/SpriteRenderer.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include <string>

RTTR_REGISTRATION
{
	rttr::registration::class_<ComboIcon>("ComboIcon")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Combo 0 Image", &ComboIcon::combo0Image)(RTTR_PUBLIC, RTTR_RESOURCE("png"))
		.property("Combo 1 Image", &ComboIcon::combo1Image)(RTTR_PUBLIC, RTTR_RESOURCE("png"))
		.property("Combo 2 Image", &ComboIcon::combo2Image)(RTTR_PUBLIC, RTTR_RESOURCE("png"))
		.property("Combo 3 Image", &ComboIcon::combo3Image)(RTTR_PUBLIC, RTTR_RESOURCE("png"));
}

//Constructor
ComboIcon::ComboIcon(World* world) : Entity(world)
{
	//Sets the default entities names
	SetName("ComboIcon");
}

void ComboIcon::Awake()
{
	AddComponents();
}

void ComboIcon::AddComponents()
{
	// Add Vox Renderer component to the entity
	m_pSpriteRenderer = GetComponent<SpriteRenderer>();
	if (!m_pSpriteRenderer)
		m_pSpriteRenderer = AddComponent<SpriteRenderer>();

	m_pSpriteRenderer->SetAlignment(RA_CENTERED);
	m_pSpriteRenderer->SetScreenAlignment(RA_CENTERED);
	m_pSpriteRenderer->SetToScreenSpace(TRUE);
	m_pSpriteRenderer->SetScaleWithScreen(TRUE);
}

void ComboIcon::SetComboIconImage(int currentComboThreshold)
{
	std::string comboImagePath;

	if (currentComboThreshold == 0)
	{
		comboImagePath = combo0Image;
	}
	if (currentComboThreshold == 1)
	{
		comboImagePath = combo1Image;
	}
	if (currentComboThreshold == 2)
	{
		comboImagePath = combo2Image;
	}
	if (currentComboThreshold > 2)
	{
		comboImagePath = combo3Image;
	}
	
	if (comboImagePath != "")
	{
		m_pSpriteRenderer->SetFilePath(comboImagePath);
	}
}
