#pragma once
#include "Core/ECS/Entity.h"

class World;
class Camera;
class ViewPoint : public Entity 
{
public:
	ViewPoint(World* pWorld): Entity(pWorld) {}

	void Awake() override;
	void SetViewTarget(Camera* pCamera);

private:
	bool m_bIsSet = false;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};