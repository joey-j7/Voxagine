#include "DashCooldownComponent.h"

#include "Humanoids/Players/Player.h"
#include "Core/ECS/Components/UI/UISlider.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<DashCooldownComponent>("Dash Cooldown UI")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Player", &DashCooldownComponent::m_pPlayer) (RTTR_PUBLIC)
		.property("Cooldown Slider", &DashCooldownComponent::m_pDashCooldownSlider) (RTTR_PUBLIC)
	;
}
DashCooldownComponent::DashCooldownComponent(Entity* pOwner) 
	: BehaviorScript(pOwner)
{
}

void DashCooldownComponent::Tick(float fDeltaTime)
{
	BehaviorScript::Tick(fDeltaTime);

	if (m_pPlayer && m_pDashCooldownSlider)
		m_pDashCooldownSlider->SetValue(1.f - (m_pPlayer->GetCurrentDashCooldownTimer() / m_pPlayer->GetDashCooldownDuration()));
}