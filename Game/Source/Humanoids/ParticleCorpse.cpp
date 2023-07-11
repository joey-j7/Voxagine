#include "pch.h"
#include "ParticleCorpse.h"

#include "Core/ECS/World.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"
#include "Core/ECS/Components/Particles/Emitters/VoxFrameEmitter.h"
#include "Core/ECS/Components/Particles/Modules/CollisionModule.h"
#include "Core/ECS/Components/Particles/Modules/AttractorModule.h"
#include "Core/ECS/Components/Particles/Modules/BasicTimerModule.h"

#include "External/rttr/registration.h"
#include <Core/MetaData/PropertyTypeMetaData.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<ParticleCorpse>("Particle Corpse")
	.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Vox Model File", &ParticleCorpse::m_voxFile)(RTTR_PUBLIC)
	.property("Particle Life Time", &ParticleCorpse::m_fParticleLifeTime)(RTTR_PUBLIC);
}

ParticleCorpse::ParticleCorpse(World * world) :
	Entity(world),
	m_voxFile("")	
{
	SetName("Particle Corpse");
}

void ParticleCorpse::Awake()
{
	Entity::Awake();

	// VoxRenderer
	VoxRenderer* pRenderer = GetComponent<VoxRenderer>();
	if (pRenderer == nullptr)
	{
		pRenderer = AddComponent<VoxRenderer>();
		if (pRenderer) 
			pRenderer->SetModelFilePath(m_voxFile);
	}	

	// ParticleSystem
	ParticleSystem* pSystem = GetComponent<ParticleSystem>();
	if (pSystem == nullptr)
		pSystem = AddComponent<ParticleSystem>();

	if (!m_bParticlesInitialized)
	{
		m_bParticlesInitialized = true;
		if (pSystem != nullptr)
		{
			pSystem->SetCustomSetup(true);
			pSystem->AddModule(new CollisionModule(pSystem));
			pSystem->AddModule(new AttractorModule(pSystem));
		}

		VoxFrameEmitter* pEmitter = new VoxFrameEmitter(pSystem);
		if (pRenderer != nullptr)
		{
			pEmitter->SetMinForce(m_MinForce);
			pEmitter->SetMaxForce(m_MaxForce);
			pEmitter->SetArcAngle(m_fArcAngle);
			pEmitter->SetSplashDirection(m_SplashDirection);
			pEmitter->SetVoxRenderer(pRenderer);
		}

		if (pSystem != nullptr)
			pSystem->SetParticleEmitter(pEmitter);
	}
}

void ParticleCorpse::Start()
{
	Entity::Start();

	// VoxRenderer
	VoxRenderer* pRenderer = GetComponent<VoxRenderer>();
	if (pRenderer == nullptr)
	{
		pRenderer = AddComponent<VoxRenderer>();
	}
	if (pRenderer != nullptr)
		pRenderer->SetModelFilePath(m_voxFile);

	// ParticleSystem
	ParticleSystem* pSystem = GetComponent<ParticleSystem>();
	if (pSystem == nullptr)
		pSystem = AddComponent<ParticleSystem>();

	if (!m_bParticlesInitialized)
	{
		m_bParticlesInitialized = true;
		if (pSystem != nullptr)
		{
			pSystem->SetCustomSetup(true);
			pSystem->AddModule(new CollisionModule(pSystem));
			pSystem->AddModule(new AttractorModule(pSystem));
		}

		VoxFrameEmitter* pEmitter = new VoxFrameEmitter(pSystem);
		if (pRenderer != nullptr)
		{
			pEmitter->SetMinForce(m_MinForce);
			pEmitter->SetMaxForce(m_MaxForce);
			pEmitter->SetArcAngle(m_fArcAngle);
			pEmitter->SetSplashDirection(m_SplashDirection);
			pEmitter->SetVoxRenderer(pRenderer);
		}

		if (pSystem != nullptr)
			pSystem->SetParticleEmitter(pEmitter);
	}
}

void ParticleCorpse::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);
	m_fTimer += fDeltaTime;

	VoxRenderer* pRenderer = GetComponent<VoxRenderer>();
	ParticleSystem* pSystem = GetComponent<ParticleSystem>();

	// Spawn particles
	if (!m_bParticlesSpawned && !m_voxFile.empty() && pRenderer != nullptr && pSystem != nullptr)
	{
		m_bParticlesSpawned = true;
		Vector3 bounds = pRenderer->GetBounds().GetSize();
		pRenderer->SetEnabled(false);
		pSystem->Emit(bounds.x * bounds.y * bounds.z);
	}

	if (pSystem != nullptr && (!pSystem->IsPlaying() || m_fTimer > m_fParticleLifeTime))
		Destroy();
}
