#include "Hum_DashState.h"

#include "Humanoids/Players/Player.h"
#include "Core/ECS/Components/Transform.h"
#include <Core/ECS/Components/VoxAnimator.h>
#include "Core/ECS/Components/PhysicsBody.h"

#include "Core/ECS/Components/AudioSource.h"
#include "Core/ECS/Components/AudioPlaylist.h"

void Hum_DashState::Start(Player* pOwner)
{
	AudioSource* pAudio = pOwner->GetComponent<AudioSource>();

	pAudio->SetFilePath("Content/SFX/PlayerDash.ogg");
	pAudio->Play();

	// Set moving animation
	if (!pOwner->m_dashAnimation.empty())
	{
		pOwner->GetComponent<VoxAnimator>()->SetFPS(pOwner->m_iDashAnimationFPS);
		pOwner->GetComponent<VoxAnimator>()->SetCurrentAnimationFile(pOwner->m_dashAnimation);
	}

	// Calculate forward based on current velocity
	m_v3Forward = pOwner->GetVelocity();
	m_v3Forward.y = 0.f;

	m_v3Forward = glm::normalize(m_v3Forward);

	// Rotate to forward vector
	float fAngle = atan2(m_v3Forward.x, m_v3Forward.z);
	Quaternion quat;

	quat.x = 0.f;
	quat.y = std::sin(fAngle * 0.5f);
	quat.z = 0.f;
	quat.w = std::cos(fAngle * 0.5f);

	pOwner->GetTransform()->SetRotation(quat);

	// Reset timer
	m_fTimer = pOwner->GetDashDuration();
	pOwner->m_bIsDashing = true;
}

void Hum_DashState::Tick(Player* pOwner, float fDeltaTime)
{
	m_fTimer -= fDeltaTime;

	if (m_fTimer <= 0.f)
	{
		pOwner->SetState("Idle");
		PhysicsBody* pBody = pOwner->GetComponent<PhysicsBody>();
		pBody->SetVelocity(Vector3(0, 0, 0));
	}	
}

void Hum_DashState::FixedTick(Player* pOwner, const GameTimer& gameTimer) {
	PhysicsBody* pBody = pOwner->GetComponent<PhysicsBody>();
	pBody->SetVelocity(m_v3Forward * pOwner->GetDashSpeed());
}

void Hum_DashState::Exit(Player * pOwner)
{
	pOwner->m_bIsDashing = false;
}
