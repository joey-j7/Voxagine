#include "pch.h"
#include "AutoMoveMonster.h"

#include <External/rttr/registration.h>
#include <Core/Application.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Components/VoxAnimator.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<AutoMoveMonster>("AutoMoveMonster")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

AutoMoveMonster::AutoMoveMonster(World* world) :
	Entity(world)
{}

void AutoMoveMonster::Awake()
{
	Entity::Awake();
	SetName("AutoMoveMonster");
}

void AutoMoveMonster::Start()
{
	Entity::Start();

	const auto& players = GetWorld()->FindEntitiesWithTag("Player");

	if (!players.empty())
		m_pPlayer = players.front();
}

void AutoMoveMonster::FixedTick(const GameTimer& gameTimer)
{
	Entity::FixedTick(gameTimer);

	if (!m_pPlayer)
		return;

	Vector3 dir = glm::normalize(m_pPlayer->GetTransform()->GetPosition() - GetTransform()->GetPosition());
	GetTransform()->Translate(dir * 20.0f * (float)gameTimer.GetElapsedSeconds());

	GetTransform()->SetRotation(glm::quatLookAt(dir, GetTransform()->GetUp()));
}