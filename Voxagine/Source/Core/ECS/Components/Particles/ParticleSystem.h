#pragma once
#include "Core/ECS/Component.h"
#include "Core/ECS/Components/Particles/ParticlePool.h"

#include <External/rttr/type>

struct VoxFrame;
class ParticleModule;
class ParticleEmitter;
class ParticleSystem : public Component
{
public:
	friend class PhysicsSystem;

	ParticleSystem(Entity* pOwner);
	~ParticleSystem();

	void Start() override;
	void Tick(float fDeltaTime);
	
	void Emit(uint32_t uiNumParticles);
	void Play();
	void Stop();

	void AddModule(ParticleModule* pModule) { m_ParticleModules.push_back(pModule); }

	void SetParticleEmitter(ParticleEmitter* pEmitter);
	ParticleEmitter* GetParticleEmitter() { return m_pParticleEmitter; }
	ParticlePool& GetParticles() { return m_Particles; }

	void SetMaxParticles(uint32_t uiMaxParticles) { m_uiMaxParticles = uiMaxParticles; }
	uint32_t GetMaxParticles() const { return m_uiMaxParticles; }

	void SetLooping(bool bLooping) { m_bLooping = bLooping; }
	bool IsLooping() const { return m_bLooping; }

	void SetSimulationTime(float fTime) { m_fSimulationTime = fTime; }
	float GetSimulationTime() const { return m_fSimulationTime; }

	void SetParticleLifeTime(float fTime) { m_fParticleLifeTime = fTime; }
	float GetParticleLifeTime() const { return m_fParticleLifeTime; }

	void SetParticleStartSpeed(float fSpeed) { m_fParticleStartSpeed = fSpeed; }
	float GetParticleStartSpeed() const { return m_fParticleStartSpeed; }

	void SetEmissionRate(uint32_t uiRate) { m_uiEmissionRate = uiRate; }
	uint32_t GetEmissionRate() const { return m_uiEmissionRate; }

	void SetParticleStartColor(VColor color) { m_ParticleStartColor = color; }
	VColor GetParticleStartColor() const { return m_ParticleStartColor; }

	void SetBoxEmitterSize(Vector3 boxSize);
	Vector3 GetBoxEmitterSize() const { return m_BoxEmitterSize; }

	void SetCustomSetup(bool bSetup) { m_bCustomSetup = bSetup; }

	bool IsPlaying() const { return m_bIsPlaying; }

private:
	ParticlePool m_Particles;
	ParticleEmitter* m_pParticleEmitter = nullptr;
	std::vector<ParticleModule*> m_ParticleModules;
	
	uint32_t m_uiMaxParticles = 5000;
	bool m_bLooping = true;
	bool m_bCustomSetup = false;
	float m_fSimulationTime = 5.f;
	float m_fParticleLifeTime = 5.f;
	float m_fParticleStartSpeed = 5.f;
	uint32_t m_uiEmissionRate = 10;
	VColor m_ParticleStartColor = VColor((unsigned char)255, 255, 255, 255);

	Vector3 m_BoxEmitterSize = Vector3(10, 10, 10);

	bool m_bIsPlaying = true;
	float m_fSimulationTimer = 0.f;

	RTTR_ENABLE(Component)
};