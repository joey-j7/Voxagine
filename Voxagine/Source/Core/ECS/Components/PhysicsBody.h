#pragma once
#include "Core/Event.h"
#include "Core/ECS/Component.h"

#include "Core/Math.h"

#include <External/rttr/type>

struct Voxel;
class PhysicsSystem;
class BoxCollider;
class PhysicsBody : public Component
{
public:
	friend class PhysicsSystem;

	PhysicsBody(Entity* pOwner);

	static const Vector3 GRAVITY;

	void ApplyForce(Vector3 force);
	void ApplyImpulse(Vector3 impulse);

	void OnEnabled() override;
	void OnDisabled() override;

	void Awake() override;
	void Tick(float fDeltaTime);

	BoxCollider* GetCollider() const { return m_pCollider; }
	Vector3 GetVelocity() const { return m_Velocity; }
	bool HasGravity() const { return m_bGravity; }
	float GetInvMass() const { return m_fInvMass; }
	float GetMass() const { return m_fMass; }
	bool IsResting() const { return m_bIsResting; }
	uint32_t GetStepHeight() const { return m_uiStepHeight; }
	
	void SetVelocity(Vector3 velocity) { m_Velocity = velocity; }
	void SetGravity(bool bGravity) { m_bGravity = bGravity; }
	void SetInvMass(float fInvMass) { m_fInvMass = fInvMass; }
	void SetMass(float fMass);
 	void SetStepHeight(uint32_t uiStepHeight) { m_uiStepHeight = uiStepHeight; }
	void SetResting(bool bResting) { m_bIsResting = bResting; }

private:
	BoxCollider* m_pCollider;

	Vector3 m_Velocity = Vector3(0.0f);
	Vector3 m_Force = Vector3(0.0f);

	float m_fMass;
	float m_fInvMass;
	float m_fDrag;
	uint32_t m_uiStepHeight;

	bool m_bIsResting;
	bool m_bGravity;

	RTTR_ENABLE(Component)
};