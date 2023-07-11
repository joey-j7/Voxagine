#include "Hum_IdleState.h"

#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/Resources/Formats/VoxModel.h"
#include <Core/ECS/Components/VoxAnimator.h>
#include "Humanoids/Players/Player.h"

void Hum_IdleState::Start(Player* pOwner)
{
	// Idle animation
	if (!pOwner->m_idleAnimation.empty())
	{
		pOwner->GetComponent<VoxAnimator>()->SetFPS(pOwner->m_iIdleAnimationFPS);
		pOwner->GetComponentAll<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_idleAnimation);
	}
}

void Hum_IdleState::Tick(Player* pOwner, float fDeltaTime)
{
	Vector3 direction = pOwner->GetDirection();
	Vector2 playerrotinput = pOwner->GetRotationInput();

	// Rotate towards direction
	if (playerrotinput != Vector2(0.f, 0.f))
	{
		direction.y = 0.f;
		direction = glm::normalize(direction);

		float fAngle = atan2(direction.x, direction.z);
		Quaternion quat;

		quat.x = 0.f;
		quat.y = std::sin(fAngle * 0.5f);
		quat.z = 0.f;
		quat.w = std::cos(fAngle * 0.5f);

		pOwner->GetTransform()->SetRotation(quat);
	}

	if (pOwner->GetVelocity() != Vector3(0.f, 0.f, 0.f))
	{
		pOwner->SetState("Moving");
	}
}
