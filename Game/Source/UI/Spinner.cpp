#include "Spinner.h"

#include "Core/ECS/Components/SpriteRenderer.h"
#include "Core/ECS/Components/Transform.h"

#include <External/rttr/registration>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Spinner>("UI Spinner")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Spinner Sprite", &Spinner::m_pSpinnerSprite) (RTTR_PUBLIC)
		.property("Degree Per Sec", &Spinner::m_fRotationDegreePerSec) (RTTR_PUBLIC)
		.property("Rotation Axises", &Spinner::m_RotationAxis) (RTTR_PUBLIC)
	;
}

Spinner::Spinner(Entity* pEntity)
	: BehaviorScript(pEntity)
{
}

Spinner::~Spinner()
{
}

void Spinner::Tick(float fDeltaTime)
{
	if (m_pSpinnerSprite)
		m_pSpinnerSprite->GetTransform()->Rotate(m_RotationAxis * m_fRotationDegreePerSec * fDeltaTime, true);
}
