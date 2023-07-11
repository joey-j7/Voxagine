#include "TestDummy.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"
#include "Core/ECS/Components/Particles/Emitters/VoxFrameEmitter.h"
#include "Core/ECS/Components/VoxRenderer.h"

#include "Core/ECS/World.h"

#include "External/rttr/registration.h"
#include "Core/ECS/Components/Particles/Modules/CollisionModule.h"
#include "Core/ECS/Components/Particles/Modules/AttractorModule.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<TestDummy>("TestDummy")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr);
}

TestDummy::TestDummy(World* world) :
	Entity(world)
{

}

void TestDummy::Awake()
{
	Entity::Awake();

	VoxRenderer* pRenderer = AddComponent<VoxRenderer>();
	if (pRenderer)
		pRenderer->SetModelFilePath("Content/Models/Props/15x15x15.vox");

	ParticleSystem* pSystem = AddComponent<ParticleSystem>();
	if (pSystem)
	{
		VoxFrameEmitter* pEmitter = new VoxFrameEmitter(pSystem);
		pEmitter->SetVoxRenderer(pRenderer);
		pSystem->SetParticleEmitter(pEmitter);
	}
}

void TestDummy::Start()
{
	Entity::Start();

	ParticleSystem* pSystem = GetComponent<ParticleSystem>();
	if (pSystem)
	{
		pSystem->AddModule(new CollisionModule(pSystem));
		pSystem->AddModule(new AttractorModule(pSystem));

		VoxFrameEmitter* pEmitter = new VoxFrameEmitter(pSystem);
		pEmitter->SetVoxRenderer(GetComponent<VoxRenderer>());
		pSystem->SetParticleEmitter(pEmitter);
	}
}

void TestDummy::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);

	m_fTimer += fDeltaTime;
	if (m_fTimer > 5.f)
	{
		m_fTimer = 0.f;
		VoxRenderer* pRenderer = GetComponent<VoxRenderer>();
		Vector3 bounds = pRenderer->GetBounds().GetSize();
		GetComponent<ParticleSystem>()->Emit(bounds.x * bounds.y * bounds.z);
	}
}
