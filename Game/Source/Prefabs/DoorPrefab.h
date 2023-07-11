#pragma once
#include "Core/ECS/Entity.h"

class DoorPrefab : public Entity
{
public:
	DoorPrefab(World* world) : Entity(world) {}

	void SetWorld(const std::string& levelName);

	void Start() override;
	void Tick(float fDeltaTime) override;

	void OnCollisionEnter(Collider*, const Manifold&) override;
	void OnCollisionStay(Collider*, const Manifold&) override;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
private:
	bool m_bWon = false;
	std::string m_LevelName = "";
	float m_fWinResetTimer = 10.0f;
	float m_fWinTimer = m_fWinResetTimer;
};
