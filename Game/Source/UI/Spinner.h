#pragma once

#include "Core\ECS\Components\BehaviorScript.h"

#include "Core/Math.h"

class SpriteRenderer;
class Spinner :
	public BehaviorScript
{
public:

	Spinner(Entity* pEntity);
	virtual ~Spinner();

	void Tick(float) override;

private:

	SpriteRenderer* m_pSpinnerSprite = nullptr;

	float m_fRotationDegreePerSec = 180.f;
	Vector3 m_RotationAxis = Vector3(0.f, 0.f, 1.f);

	RTTR_ENABLE(BehaviorScript)
	RTTR_REGISTRATION_FRIEND
};

