#include "pch.h"
#include "LongNeckMonster.h"

#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include <External/rttr/registration.h>
#include <Core/MetaData/PropertyTypeMetaData.h>
#include "Core/ECS/Components/AudioSource.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<LongNeckMonster>("Long Neck Lady Monster")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

LongNeckMonster::LongNeckMonster(World * world) :
	Monster(world)
{
	SetName("Long Neck Lady Monster");
}

void LongNeckMonster::Awake()
{
	Monster::Awake();
}

void LongNeckMonster::Start()
{
	Monster::Start();
}

void LongNeckMonster::Tick(float fDeltaTime)
{
	Monster::Tick(fDeltaTime);
}

void LongNeckMonster::ApplyDefaultValues()
{
	m_fWakeUpRange = 200.f;
	m_fWakeUpTime = 0.6f;
	m_bCanMeleeAttack = true;
	m_fAttackCooldown = 2.f;
	m_fMeleeAttackTime = 1.5f;
	m_fMeleeRange = 60.f;
	m_fDamageKnockBack = 30.f;

	m_idleAnimation = "Content/Character_Models/Long_Neck_Lady/Long_Neck_Eyes_Closed.anim.vox";
	m_movingAnimation = "Content/Character_Models/Long_Neck_Lady/Long_Neck_Eyes_Open.anim.vox";
	m_wakeUpAnimation = "Content/Character_Models/Long_Neck_Lady/Long_Neck_Neck_Extend_Up.anim.vox";
	m_meleeAttackAnimation = "Content/Character_Models/Long_Neck_Lady/Long_Neck_Anticipation_Attack.anim.vox";

	pathfinding::Pathfinder* pathfinder = GetComponentAll<pathfinding::Pathfinder>();
	if (pathfinder != nullptr)
	{
		pathfinder->m_fMinVelocity = 160;
		pathfinder->m_fMaxVelocity = 160;
	}
}

void LongNeckMonster::MeleeAttack(Vector3& velocity)
{
	Monster::MeleeAttack(velocity);
}