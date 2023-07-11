#pragma once
#include "Core/ECS/Components/Particles/ParticleEmitter.h"
#include "Core/Math.h"

class VoxRenderer;
class VoxFrameEmitter : public ParticleEmitter
{
public:
	VoxFrameEmitter(ParticleSystem* pSystem) : ParticleEmitter(pSystem) {}

	virtual void Emit(float fDeltatime, ParticlePool& particleData, uint32_t uiStartId, uint32_t uiEndId) override;

	void SetVoxRenderer(VoxRenderer* pRenderer) { m_pRenderer = pRenderer; }
	VoxRenderer* GetVoxRenderer() { return m_pRenderer; }

	Vector3 GetMinForce() const { m_MinForce; }
	void SetMinForce(Vector3 force) { m_MinForce = force; }

	Vector3 GetMaxForce() const { m_MaxForce; }
	void SetMaxForce(Vector3 force) { m_MaxForce = force; }

	void SetArcAngle(float fArc) { m_fArcAngle = fArc; }
	void SetSplashDirection(Vector3 dir) { m_SplashDirection = dir; }

private:
	VoxRenderer* m_pRenderer = nullptr;
	Vector3 m_MinForce = Vector3(0.f);
	Vector3 m_MaxForce = Vector3(0.f);
	Vector3 m_SplashDirection = Vector3(0.f);
	float m_fArcAngle = 0.f;
};