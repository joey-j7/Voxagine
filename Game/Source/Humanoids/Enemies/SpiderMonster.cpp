#include "pch.h"
#include "SpiderMonster.h"

#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include <External/rttr/registration.h>
#include <Core/MetaData/PropertyTypeMetaData.h>

#include "Core/ECS/Components/AudioSource.h"
#include "Humanoids/Players/Player.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<SpiderMonster>("Spider Monster")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

SpiderMonster::SpiderMonster(World * world) :
	Monster(world)
{
	SetName("Spider Monster");
	
	m_bulletModelFile = "Content/Character_Models/Spider/Spider_Silk_Projectile_Loop.anim.vox";
	m_idleAnimation = "Content/Character_Models/Spider/Spider_Idle.anim.vox";
	m_movingAnimation = "Content/Character_Models/Spider/Spider_Walk.anim.vox";
	m_rangeAttackAnimation = "Content/Character_Models/Spider/Spider_Attack.anim.vox";
}

void SpiderMonster::Awake()
{
	Monster::Awake();

	m_fWakeUpTime = 0.f;
	m_fBulletSpeed = 170.f;
	m_fShootingRange = 175.f;
	m_bCanRangeAttack = true;
	m_fMeleeRange = 70.f;

	pathfinding::Pathfinder* pathfinder = GetComponentAll<pathfinding::Pathfinder>();
	if (pathfinder != nullptr)
	{
		pathfinder->m_fMinVelocity = 120;
		pathfinder->m_fMaxVelocity = 120;
	}
}

void SpiderMonster::Start()
{
	Monster::Start();
}

void SpiderMonster::Tick(float fDeltaTime)
{
	Monster::Tick(fDeltaTime);

	pathfinding::Pathfinder* pathfinder = GetComponentAll<pathfinding::Pathfinder>();
	if (pathfinder != nullptr && !m_bIsIdle)
	{
		std::vector<Player*> players = GetWorld()->FindEntitiesOfType<Player>();
		for (Player* player : players)
		{
			float distance2 = glm::length2(player->GetTransform()->GetPosition() - GetTransform()->GetPosition());
			if (distance2 < m_fMeleeRange *  m_fMeleeRange)
				pathfinder->m_applyVelocity = false;
			else
				pathfinder->m_applyVelocity = true;
		}
	}
}