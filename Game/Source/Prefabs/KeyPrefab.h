#pragma once
#include "Core/ECS/Entity.h"

class KeyPrefab : public Entity
{
public:
	KeyPrefab(World* world);

	void SetTriggerMethod(std::function<bool()> triggerMethod);
	void OnCollisionEnter(Collider*, const Manifold&) override;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
private:
	std::function<bool()> m_TriggerMethod;
};
