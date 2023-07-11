#include "Hum_MoveState.h"

#include "Humanoids/Players/Player.h"
#include "Core/ECS/Components/Transform.h"
#include <Core/ECS/Components/VoxAnimator.h>
#include "Core/ECS/Components/PhysicsBody.h"

void Hum_MoveState::Start(Player* pOwner)
{
	// Moving
	if (!pOwner->m_movingAnimation.empty())
	{
		pOwner->GetComponent<VoxAnimator>()->SetFPS(pOwner->m_iMovingAnimationFPS);
		pOwner->GetComponent<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_movingAnimation);
	}
}

void Hum_MoveState::Tick(Player* pOwner, float fDeltaTime)
{
	if (pOwner->GetVelocity() == Vector3(0.f, 0.f, 0.f))
	{
		pOwner->SetState("Idle");
	}
}

void Hum_MoveState::FixedTick(Player* pOwner, const GameTimer& gameTimer)
{

	Vector3 v3Velocity = pOwner->GetVelocity();

	PhysicsBody* physBody = pOwner->GetComponent<PhysicsBody>();

	if (physBody)
	{
		// Move
		Vector3 tempVel = v3Velocity;
		tempVel.y = physBody->GetVelocity().y;
		physBody->SetVelocity(tempVel);
	}

	// Rotate towards direction
	Vector3 v3Direction = pOwner->GetDirection();
	float fAngle = 0.f;

	if (v3Direction != Vector3(0.f, 0.f, 0.f))
	{
		v3Direction.y = 0.f;
		v3Direction = glm::normalize(v3Direction);

		fAngle = atan2(v3Direction.x, v3Direction.z);
	}

	// Rotate towards velocity's direction if no direction has been specified
	else
	{
		v3Velocity.y = 0.f;
		v3Velocity = glm::normalize(v3Velocity);

		fAngle = atan2(v3Velocity.x, v3Velocity.z);
	}

	Quaternion quat;

	quat.x = 0.f;
	quat.y = std::sin(fAngle * 0.5f);
	quat.z = 0.f;
	quat.w = std::cos(fAngle * 0.5f);

	pOwner->GetTransform()->SetRotation(quat);
}