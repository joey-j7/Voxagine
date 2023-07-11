#include "FlashBehavior.h"
#include <Core/ECS/Components/VoxRenderer.h>

#include "Core/MetaData/PropertyTypeMetaData.h"

#include <External/rttr/registration>

RTTR_REGISTRATION
{
	rttr::registration::class_<FlashBehavior>("FlashBehavior")
			.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
			.property("Flashing Color", &FlashBehavior::FlashingColor, rttr::registration::public_access) (RTTR_PUBLIC)
			.property("Flash Delay", &FlashBehavior::dFlashDelay, rttr::registration::public_access) (RTTR_PUBLIC)
			.property("Times to flash", &FlashBehavior::iTimesToFlash, rttr::registration::public_access) (RTTR_PUBLIC)
			.property("Current Color", &FlashBehavior::m_CurrentColor, rttr::registration::private_access) (RTTR_PRIVATE);
}

void FlashBehavior::Awake()
{
	m_pRenderer = GetOwner()->GetComponent<VoxRenderer>();
	if (m_pRenderer)
	{
		m_CurrentColor = m_pRenderer->GetOverrideColor();
	}
}

void FlashBehavior::StartFlashing()
{
	if(!m_bStarted)
	{
		m_bStarted = true;
		StartRoutine(std::bind(&FlashBehavior::Flash, this), "Flash");
	}
}

double FlashBehavior::Flash()
{
	// if amount of flash count is not set continue
	if (m_iCurrentCount > iTimesToFlash) {
		m_pRenderer->SetOverrideColor(m_CurrentColor);
		m_bFlashing = m_bStarted = false;
		m_iCurrentCount = 0;

		return STOP_ROUTINE;
	}

	// if we are not flashing. Start flashing
	if(!m_bFlashing)
	{
		// Set flash
		m_pRenderer->SetOverrideColor(FlashingColor);
	} else {
		m_pRenderer->SetOverrideColor(m_CurrentColor);
	}


	// Toggle the flasher
	m_bFlashing = !m_bFlashing;
	// increment counter
	m_iCurrentCount++;

	// return the time to delay
	return dFlashDelay;
}


