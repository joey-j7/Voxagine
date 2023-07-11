#include "pch.h"
#include "ParticleSystem.h"
#include "Core/ECS/Components/Particles/ParticleEmitter.h"
#include "Core/ECS/Components/Particles/ParticleModule.h"

#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Core/Platform/Rendering/RenderContext.h"

#include "Core/ECS/Components/Particles/Emitters/BoxEmitter.h"
#include "Core/ECS/Components/Particles/Modules/BasicTimerModule.h"

#include <External/rttr/registration>
#include "External/rttr/policy.h"
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<ParticleSystem>("ParticleSystem")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("MaxParticles", &ParticleSystem::GetMaxParticles, &ParticleSystem::SetMaxParticles)(RTTR_PUBLIC)
		.property("Looping", &ParticleSystem::IsLooping, &ParticleSystem::SetLooping)(RTTR_PUBLIC)
		.property("SimulationTime", &ParticleSystem::GetSimulationTime, &ParticleSystem::SetSimulationTime)(RTTR_PUBLIC)
		.property("ParticleLifeTime", &ParticleSystem::GetParticleLifeTime, &ParticleSystem::SetParticleLifeTime)(RTTR_PUBLIC)
		.property("ParticleStartSpeed", &ParticleSystem::GetParticleStartSpeed, &ParticleSystem::SetParticleStartSpeed)(RTTR_PUBLIC)
		.property("ParticleStartColor", &ParticleSystem::GetParticleStartColor, &ParticleSystem::SetParticleStartColor)(RTTR_PUBLIC)
		.property("EmissionRate", &ParticleSystem::GetEmissionRate, &ParticleSystem::SetEmissionRate)(RTTR_PUBLIC)
		.property("BoxEmitterSize", &ParticleSystem::GetBoxEmitterSize, &ParticleSystem::SetBoxEmitterSize)(RTTR_PUBLIC);
}

ParticleSystem::ParticleSystem(Entity * pOwner) :
	Component(pOwner)
{
}

ParticleSystem::~ParticleSystem()
{
	delete m_pParticleEmitter;
	for (ParticleModule* pModule : m_ParticleModules)
	{
		delete pModule;
	}
}

void ParticleSystem::Start()
{
	m_Particles.Create(m_uiMaxParticles);

	if (!m_bCustomSetup)
	{
		SetParticleEmitter(new BoxEmitter(this));
		static_cast<BoxEmitter*>(m_pParticleEmitter)->SetBoxSize(m_BoxEmitterSize);
		m_ParticleModules.push_back(new BasicTimerModule(this));
	}
}

void ParticleSystem::Tick(float fDeltaTime)
{
	m_fSimulationTimer += fDeltaTime;
	if (!m_bLooping && m_fSimulationTimer >= m_fSimulationTime)
		Stop();

	if (m_bIsPlaying)
		m_pParticleEmitter->Tick(fDeltaTime);

	for (ParticleModule* pModule : m_ParticleModules)
	{
		pModule->Tick(fDeltaTime, m_Particles);
	}
}

void ParticleSystem::Emit(uint32_t uiNumParticles)
{
	uint32_t startId = m_Particles.GetNumAliveParticles();
	uint32_t endId = std::min(startId + uiNumParticles, m_Particles.GetNumParticles() - 1);

	m_pParticleEmitter->Emit(GetWorld()->GetDeltaSeconds(), m_Particles, startId, endId);
}

void ParticleSystem::Play()
{
	m_bIsPlaying = true;
}

void ParticleSystem::Stop()
{
	m_bIsPlaying = false;
	m_fSimulationTimer = 0.f;
}

void ParticleSystem::SetParticleEmitter(ParticleEmitter* pEmitter)
{
	if (m_pParticleEmitter)
		delete m_pParticleEmitter;

	m_pParticleEmitter = pEmitter;
}

void ParticleSystem::SetBoxEmitterSize(Vector3 boxSize)
{
	m_BoxEmitterSize = boxSize;
	if (BoxEmitter* pEmitter = dynamic_cast<BoxEmitter*>(m_pParticleEmitter))
	{
		pEmitter->SetBoxSize(boxSize);
	}
}
