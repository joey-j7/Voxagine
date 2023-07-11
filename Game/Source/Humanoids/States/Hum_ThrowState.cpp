#include "pch.h"
#include "Hum_ThrowState.h"
#include <Core/ECS/Components/VoxAnimator.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include "Weapons/Weapon.h"

Hum_ThrowState::Hum_ThrowState()
{

}
void Hum_ThrowState::Start(Player* pOwner) 
{
	m_fTimer = pOwner->m_fThrowAnimationTime;
	int subtract;
	// Shoot
	pOwner->GetComponent<VoxAnimator>()->SetFPS(pOwner->m_iThrowAnimationFPS);
	if (!pOwner->m_throwAnimation.empty())
		pOwner->GetComponent<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_throwAnimation);	
}
void Hum_ThrowState::Tick(Player* pOwner, float fDeltaTime)  
{
	m_fTimer -= fDeltaTime;
	if (m_fTimer <= 0.f)
	{
		pOwner->GetCurrentWeapon()->Fire();
		pOwner->SetState("Idle");
	}
}

void Hum_ThrowState::FixedTick(Player* pOwner, const GameTimer& gameTimer) 
{
	Vector3 velocity = pOwner->GetVelocity();

	PhysicsBody* pBody = pOwner->GetComponent<PhysicsBody>();
	pBody->SetVelocity(velocity);
}




