#include "ComboSliderUI.h"
#include "Core/ECS/World.h"
#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Core/ECS/Components/UI/UISlider.h"
#include "Core/MetaData/PropertyTypeMetaData.h"

#include <string>

RTTR_REGISTRATION
{
	rttr::registration::class_<ComboSliderUI>("ComboSliderUI")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

//Constructor
ComboSliderUI::ComboSliderUI(World* world) : Entity(world)
{
	//Sets the default entities names
	SetName("ComboSliderUI");
}

void ComboSliderUI::Awake()
{
	AddComponents();
}

void ComboSliderUI::AddComponents()
{
	// Add Vox Renderer component to the entity
	m_pComboSlider = GetComponent<UISlider>();
	if (!m_pComboSlider)
		m_pComboSlider = AddComponent<UISlider>();

	m_pComboSlider->SetValue(0.f);
}

void ComboSliderUI::SetComboSlider(int currentComboStreak, int comboGoal)
{

	if (comboGoal == 0)
	{
		if (currentComboStreak == 0)
		{
			m_pComboSlider->SetValue(0.f);
		}
		m_pComboSlider->SetValue(1.f);
		return;
	}

	else
	{
		m_pComboSlider->SetValue(static_cast<float>(currentComboStreak) / comboGoal);
	}
}