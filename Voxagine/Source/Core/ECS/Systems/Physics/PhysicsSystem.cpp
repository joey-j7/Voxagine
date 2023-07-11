#include "pch.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"

#include "Core/ECS/Components/PhysicsBody.h"
#include "Core/Resources/Formats/VoxModel.h"
#include "Core/ECS/Components/Transform.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/ECS/Systems/Physics/Manifold.h"
#include "Core/ECS/Systems/Physics/Box.h"
#include "Core/ECS/Systems/Physics/Sphere.h"
#include "Core/GameTimer.h"
#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "Core/ECS/Systems/Physics/HitResult.h"
#include "Core/Platform/Rendering/RenderContext.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Core/ECS/Entities/Camera.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Systems/Physics/IntegrityJob.h"
#include "Core/ECS/Components/Particles/ParticleSystem.h"
#include "Core/ECS/Components/Particles/ParticlePool.h"
#include <iostream>

#include "Core/Platform/Platform.h"
#include "Core/Platform/Rendering/Passes/ParticlePass.h"
#include "External/optick/optick.h"

#include <External/glm/gtx/rotate_vector.hpp>

const float PhysicsSystem::PARTICLE_DESTROY_THRESHOLD = 1.f;
const float PhysicsSystem::PARTICLE_BOUNCE_MULIPLIER = 1.5f;
const Vector3 PhysicsSystem::PARTICLE_GRAVITY = Vector3(0.f, -39.81, 0.f);

#define PHYSICS_MAGICAL_EVERYTHING_SOLVING_VALUE 0.1f

PhysicsSystem::PhysicsSystem(World* pWorld, Vector3 gridSize, uint32_t voxelSize, uint32_t uiMaxParticles, UVector3 chunkSize) :
	ComponentSystem(pWorld),
	m_ParticlePool(uiMaxParticles)

{
	m_uiMaxParticleCount = uiMaxParticles;
	m_pIntegrityJob = nullptr;
	m_pStaticEntityBody = new Entity(m_pWorld);
	m_pStaticBody = m_pStaticEntityBody->AddComponent<PhysicsBody>();
	m_pStaticBody->SetInvMass(0);

	m_VoxelGrid.Create((uint32_t)gridSize.x, (uint32_t)gridSize.y, (uint32_t)gridSize.z, voxelSize, chunkSize);

	if (m_pWorld)
	{
		m_pGPUParticles = m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->m_pParticlePass->GetMapper();
		m_pGPUParticles->Resize(m_uiMaxParticleCount, sizeof(GPUParticle));

		m_pWorld->Paused += Event<World*>::Subscriber(std::bind(&PhysicsSystem::OnWorldPaused, this, std::placeholders::_1), this);
		m_pWorld->Resumed += Event<World*>::Subscriber(std::bind(&PhysicsSystem::OnWorldResumed, this, std::placeholders::_1), this);
	}
}

PhysicsSystem::~PhysicsSystem()
{
	delete m_pStaticEntityBody;

	if (m_pIntegrityJob)
	{
		m_pIntegrityJob->Stop();
	}
}

void PhysicsSystem::Start()
{
	m_pIntegrityJob = new IntegrityJob();
	m_pIntegrityJob->SetVoxelGrid(&m_VoxelGrid);
	m_pIntegrityJob->Stopped += Event<IntegrityJob*>::Subscriber(std::bind(&PhysicsSystem::OnIntegrityJobStopped, this, std::placeholders::_1), this);
	JobQueue* pJobQueue = m_pWorld->GetJobQueue();
	if (pJobQueue)
		pJobQueue->EnqueueWithType(m_pIntegrityJob, JT_PHYSICS);
}

bool PhysicsSystem::CanProcessComponent(Component* pComponent)
{
	return dynamic_cast<PhysicsBody*>(pComponent) || dynamic_cast<BoxCollider*>(pComponent) || dynamic_cast<ParticleSystem*>(pComponent);
}

void PhysicsSystem::FixedTick(const GameTimer& fixedTimer)
{
	OPTICK_CATEGORY("Physics", Optick::Category::Physics);
	SyncIntegrityJob();

	/* Update particles */
	m_uiActiveParticleCount = 0;
	TickParticleSystems(fixedTimer);
	SimulateParticles(static_cast<float>(fixedTimer.GetElapsedSeconds()));
	if (m_uiActiveParticleCount > 0)
		m_pWorld->GetApplication()->GetPlatform().GetRenderContext()->ForceUpdate();

	ResolveContinousCollision(static_cast<float>(fixedTimer.GetElapsedSeconds()));
	TickBodies(fixedTimer);

	/* Handle collisions */
	AccumulateManifolds();
	ResolveManifolds();
	HandleCallbacks();
	
	

	for (Collider* pCollider : m_Colliders)
	{
		Vector3 gridPos = pCollider->GetTransform()->GetPosition();
		gridPos.x = round(gridPos.x);
		gridPos.y = round(gridPos.y);
		gridPos.z = round(gridPos.z);
		pCollider->SetGridPosition(gridPos);
	}
}

void PhysicsSystem::PostTick(float fDeltaTime)
{
#if defined(EDITOR) || defined(_DEBUG)
	OPTICK_CATEGORY("Physics", Optick::Category::Physics);
	for (Collider* pCollider : m_Colliders)
	{
		DebugBox box;
		box.m_Center = pCollider->GetTransform()->GetPosition();
		box.m_Color = VColors::Red;
		box.m_Extents = static_cast<BoxCollider*>(pCollider)->GetHalfBoxSize();
		m_pWorld->GetDebugRenderer()->AddBox(box);
	}
#endif
}

void PhysicsSystem::OnComponentAdded(Component* pComponent)
{
	if (PhysicsBody* pBody = dynamic_cast<PhysicsBody*>(pComponent))
	{
		if (pBody->GetOwner()->IsStatic())
			return;

		m_Bodies.push_back(pBody);
	}
	else if (BoxCollider* pCollider = dynamic_cast<BoxCollider*>(pComponent))
	{
		m_Colliders.push_back(pCollider);
	}
	else if (ParticleSystem* pParticleSystem = dynamic_cast<ParticleSystem*>(pComponent))
	{
		m_ParticleSystems.push_back(pParticleSystem);
	}
}

void PhysicsSystem::OnComponentDestroyed(Component* pComponent)
{
	if (PhysicsBody* pBody = dynamic_cast<PhysicsBody*>(pComponent))
	{
		std::vector<PhysicsBody*>::iterator iter = std::find(m_Bodies.begin(), m_Bodies.end(), pBody);
		if (iter != m_Bodies.end())
			m_Bodies.erase(iter);
	}
	else if (BoxCollider* pCollider = dynamic_cast<BoxCollider*>(pComponent))
	{
		std::vector<BoxCollider*>::iterator iter = std::find(m_Colliders.begin(), m_Colliders.end(), pCollider);
		if (iter != m_Colliders.end())
			m_Colliders.erase(iter);
	}
	else if (ParticleSystem* pParticleSystem = dynamic_cast<ParticleSystem*>(pComponent))
	{
		std::vector<ParticleSystem*>::iterator iter = std::find(m_ParticleSystems.begin(), m_ParticleSystems.end(), pParticleSystem);
		if (iter != m_ParticleSystems.end())
			m_ParticleSystems.erase(iter);
	}
}

void PhysicsSystem::OnWorldPaused(World* pWorld)
{
	if (m_pIntegrityJob)
	{
		m_pIntegrityJob->Stop();
	}
}

void PhysicsSystem::OnWorldResumed(World* pWorld)
{
	m_pIntegrityJob = new IntegrityJob();
	m_pIntegrityJob->SetVoxelGrid(&m_VoxelGrid);
	m_pIntegrityJob->Stopped += Event<IntegrityJob*>::Subscriber(std::bind(&PhysicsSystem::OnIntegrityJobStopped, this, std::placeholders::_1), this);
	JobQueue* pJobQueue = m_pWorld->GetJobQueue();
	if (pJobQueue)
		pJobQueue->EnqueueWithType(m_pIntegrityJob, JT_PHYSICS);
}

void PhysicsSystem::OnIntegrityJobStopped(IntegrityJob* pJob)
{
	m_pIntegrityJob = nullptr;
}

void PhysicsSystem::TickBodies(const GameTimer& fixedTimer)
{
	OPTICK_EVENT();
	for (PhysicsBody* pBody : m_Bodies)
	{
		if (!pBody->IsEnabled()) continue;

		/* Update body */
		pBody->Tick(static_cast<float>(fixedTimer.GetElapsedSeconds()));

		/* Clamp body to grid bounds */
		Vector3 pos = pBody->GetTransform()->GetPosition();
		if (ClampToBounds(pos, pBody->GetCollider()))
		{
			pBody->GetTransform()->SetPosition(pos);
		}

		/* Perform StepCheck on body */
		if (pBody->GetStepHeight() > 0)
		{
			const VoxFrame* pFrame = nullptr;
			VoxRenderer* pRenderer = pBody->GetOwner()->GetComponent<VoxRenderer>();

			if (pRenderer)
				pFrame = pRenderer->GetFrame();

			StepCheck(pBody, pFrame);
		}
		else pBody->SetResting(false);
	}
}

void PhysicsSystem::TickParticleSystems(const GameTimer& fixedTimer)
{
	OPTICK_EVENT();
	GPUParticle* pGPUParticles = reinterpret_cast<GPUParticle*>(m_pGPUParticles->GetData());
	GPUParticle gpuParticle;

	for (ParticleSystem* pSystem : m_ParticleSystems)
	{
		if (!pSystem->IsEnabled()) continue;

		pSystem->Tick((float)fixedTimer.GetElapsedSeconds());

		ParticlePool& pool = pSystem->GetParticles();
		for (uint32_t i = 0; i < pool.GetNumAliveParticles(); ++i)
		{
			pool.Position[i] += pool.Velocity[i] * (float)fixedTimer.GetElapsedSeconds();

			if (m_uiActiveParticleCount < m_uiMaxParticleCount)
			{
				m_uiActiveParticleCount++;

				// Create GPU particle
				gpuParticle.Position = pool.Position[i];
				gpuParticle.VoxelColor = pool.Color[i];

				std::memcpy(
					&pGPUParticles[m_uiActiveParticleCount - 1],
					&gpuParticle,
					sizeof(GPUParticle)
				);
			}
		}
	}
}

void PhysicsSystem::SyncIntegrityJob()
{
	OPTICK_EVENT();
	if (m_pIntegrityJob)
	{
		moodycamel::ReaderWriterQueue<std::vector<uint64_t>>& results = m_pIntegrityJob->GetResults();
		std::vector<uint64_t> checkedVoxels;
		while (results.try_dequeue(checkedVoxels))
		{
			for (uint64_t& vecHash : checkedVoxels) 
			{
				Vector3 vec;
				vec.x = ((uint16_t*)&vecHash)[2];
				vec.y = ((uint16_t*)&vecHash)[1];
				vec.z = ((uint16_t*)&vecHash)[0];

				Voxel* pVoxel = m_VoxelGrid.GetVoxel(
					static_cast<uint32_t>(vec.x),
					static_cast<uint32_t>(vec.y),
					static_cast<uint32_t>(vec.z));

				//If the voxel is not active we don't make it into a particle
				if (!pVoxel->Active) continue;

				Particle* pParticle = m_ParticlePool.SpawnParticle();
				if (pParticle)
				{
					pParticle->Live.BakeOnImpact = true;
					pParticle->Live.GridPosition = vec;
					pParticle->Live.Position = m_VoxelGrid.GridToWorld(vec);;
					pParticle->Live.VoxelColor = pVoxel->Color;
					pVoxel->UserPointer = (uintptr_t)pParticle;
				}

				m_VoxelGrid.ModifyVoxel(
					static_cast<int>(vec.x),
					static_cast<int>(vec.y),
					static_cast<int>(vec.z),
					0
				);
				m_pRenderSystem->ModifyVoxel(
					static_cast<uint32_t>(vec.x),
					static_cast<uint32_t>(vec.y),
					static_cast<uint32_t>(vec.z),
					0
				);

				pVoxel->Active = false;
			}
		}
	}
}



bool PhysicsSystem::RayCast(Vector3 start, Vector3 dir, HitResult& hitResult, float fLength /*= FLT_MAX*/, uint32_t uiLayer /*= CollisionLayer::CL_ALL*/)
{
	return RayCastGroup(m_Colliders, start, dir, hitResult, fLength, uiLayer);
}

bool PhysicsSystem::RayCastSingle(const BoxCollider* pCollider, Vector3 start, Vector3 dir, HitResult& hitResult, float fLength /*= FLT_MAX*/, uint32_t uiLayer /*= -1*/)
{
	if (glm::length2(dir) == 0)
		return false;

	dir = glm::normalize(dir);
	dir *= fLength;
	float invDir[3]
	{
		dir.x != 0 ? 1.f / dir.x : 0,
		dir.y != 0 ? 1.f / dir.y : 0,
		dir.z != 0 ? 1.f / dir.z : 0
	};
	float rayStart[3]{ start.x, start.y, start.z };

	float tNear = FLT_MAX;

	if ((pCollider->GetLayer() & uiLayer) == 0) return false;

	Vector3 boxMin = pCollider->GetBoxMin();
	Vector3 boxMax = pCollider->GetBoxMax();

	if (IntersectRayAABB(invDir, rayStart, (float*)&boxMin, (float*)&boxMax, tNear))
	{
		Vector3 hitPoint = start + dir * tNear;

#if defined(EDITOR) || defined(_DEBUG)							
		{
			auto debugRenderer = m_pWorld->GetDebugRenderer();
			{
				DebugLine line;
				line.m_Start = start;
				line.m_End = hitPoint;
				line.m_Color = VColors::Yellow;
				debugRenderer->AddLine(line);
			}
		}
#endif

		/* Don't hit if the distance between start and hit point are greater than ray length */
		if (glm::distance(hitPoint, start) > fLength) return false;

		/* set new hit values */
		hitResult.HitEntity = pCollider->GetOwner();
		hitResult.HitPoint = hitPoint;
		return true;
	}

	return false;
}



bool PhysicsSystem::RayCastGroup(const BoxCollider* pIgnoreCollider, std::vector<BoxCollider*>& colliders, Vector3 start, Vector3 dir, HitResult& hitResult, float fLength /*= FLT_MAX*/, uint32_t uiLayer /*= -1*/)
{
	if (glm::length2(dir) == 0)
		return false;

	dir = glm::normalize(dir);
	dir *= fLength;
	float invDir[3]
	{
		dir.x != 0 ? 1.f / dir.x : 0,
		dir.y != 0 ? 1.f / dir.y : 0,
		dir.z != 0 ? 1.f / dir.z : 0
	};
	float rayStart[3]{ start.x, start.y, start.z };

	float tNear = FLT_MAX;
	bool hit = false;

	for (BoxCollider* pBox : colliders)
	{
		if(pBox == pIgnoreCollider)
			continue;

		if ((pBox->GetLayer() & uiLayer) == 0) continue;

		Vector3 boxMin = pBox->GetBoxMin();
		Vector3 boxMax = pBox->GetBoxMax();

		if (IntersectRayAABB(invDir, rayStart, (float*)&boxMin, (float*)&boxMax, tNear))
		{
			Vector3 hitPoint = start + dir * tNear;

#if defined(EDITOR) || defined(_DEBUG)							
			{
				auto debugRenderer = m_pWorld->GetDebugRenderer();
				{
					DebugLine line;
					line.m_Start = start;
					line.m_End = hitPoint;
					line.m_Color = VColors::Yellow;
					debugRenderer->AddLine(line);
				}

				{
					DebugSphere sphere;
					sphere.m_Center = hitPoint;
					sphere.m_fRadius = 0.5f;
					sphere.m_Color = VColors::Orange;
					debugRenderer->AddSphere(sphere);
				}
			}
#endif

			/* Don't hit if the distance between start and hit point are greater than ray length */
			if (glm::distance(hitPoint, start) > fLength) continue;

			/* set new hit values */
			hit = true;
			hitResult.HitEntity = pBox->GetOwner();
			hitResult.HitPoint = hitPoint;
		}
	}
	return hit;
}

bool PhysicsSystem::RayCastGroup(std::vector<BoxCollider*>& colliders, Vector3 start, Vector3 dir, HitResult& hitResult, float fLength /*= FLT_MAX*/, uint32_t uiLayer /*= -1*/)
{
	return RayCastGroup(nullptr, colliders, start, dir, hitResult, fLength, uiLayer);
}

void PhysicsSystem::ApplySphericalDestruction(const Vector3& position, float fRadius, float fForceMin, float fForceMax, bool bBakeParticle /* true */)
{
	Vector3 clampedGridPos = m_VoxelGrid.WorldToGrid(position);
	clampedGridPos.x = round(clampedGridPos.x);
	clampedGridPos.y = round(clampedGridPos.y);
	clampedGridPos.z = round(clampedGridPos.z);

	fRadius = round(fRadius);
	uint32_t diameter = (uint32_t)fRadius * 2;

	std::vector<uint64_t> integrityChecks;
	uint32_t numVoxels = diameter * diameter * diameter;
	Voxel** voxels = new Voxel*[numVoxels];

	float fDiameter = static_cast<float>(diameter);
	bool isValid = m_VoxelGrid.GetChunk(voxels, clampedGridPos - Vector3(fRadius), Vector3(fDiameter), true);

	if (!isValid) return;

	Vector3 volumePos = clampedGridPos - Vector3(fRadius);
	volumePos.x = round(volumePos.x);
	volumePos.y = round(volumePos.y);
	volumePos.z = round(volumePos.z);

	Entity* pCachedEntity = nullptr;

	uint32_t particlesSpawned = 0;
	for (uint32_t i = 0; i < numVoxels; ++i)
	{
		++volumePos.x;
		if (volumePos.x >= clampedGridPos.x + fRadius)
		{
			volumePos.x = clampedGridPos.x - fRadius;
			++volumePos.y;
		}
		if (volumePos.y >= clampedGridPos.y + fRadius)
		{
			volumePos.y = clampedGridPos.y - fRadius;
			++volumePos.z;
		}

		if (!voxels[i] || !voxels[i]->Active || volumePos.y < 1) continue;

		Vector3 diff = volumePos - clampedGridPos;
		if (glm::length(diff) <= fRadius)
		{
			if (pCachedEntity == nullptr || voxels[i]->UserPointer != pCachedEntity->GetId())
			{
				pCachedEntity = m_pWorld->FindEntity(voxels[i]->UserPointer);
			}

			if (pCachedEntity == nullptr || pCachedEntity->IsDestructible())
			{
				uint32_t color = m_pRenderSystem->GetVoxel(
					static_cast<int32_t>(volumePos.x - 1),
					static_cast<int32_t>(volumePos.y),
					static_cast<int32_t>(volumePos.z)
				);

				if (particlesSpawned % 4 == 0)
				{
					Particle* pParticle = m_ParticlePool.SpawnParticle();
					if (pParticle)
					{
						pParticle->Live.GridPosition = volumePos;
						pParticle->Live.Position = m_VoxelGrid.GridToWorld(volumePos);
						pParticle->Live.BakeOnImpact = bBakeParticle;

						diff = glm::normalize(diff);
						pParticle->Live.Velocity = diff * glm::linearRand(fForceMin, fForceMax);
						pParticle->Live.VoxelColor = color;

						voxels[i]->UserPointer = (uintptr_t)pParticle;
					}
				}
				++particlesSpawned;

				voxels[i]->Active = false;

				int32_t volX, volY, volZ;
				volX = static_cast<int32_t>(volumePos.x - 1);
				volY = static_cast<int32_t>(volumePos.y);
				volZ = static_cast<int32_t>(volumePos.z);
				m_VoxelGrid.ModifyVoxel(volX, volY, volZ, 0);
				m_pRenderSystem->ModifyVoxel(volX, volY, volZ, 0);

				for (float x = -1.0f; x <= 1.0f; ++x)
				{
					for (float z = -1.0f; z <= 1.0f; ++z)
					{
						Vector3 vec = volumePos + Vector3(x, 1, z);

						uint64_t hash = 0;
						((uint16_t*)&hash)[0] = (uint16_t)vec.z;
						((uint16_t*)&hash)[1] = (uint16_t)vec.y;
						((uint16_t*)&hash)[2] = (uint16_t)vec.x;
						integrityChecks.push_back(hash);
					}
				}
			}
		}
	}

	m_pIntegrityJob->EnqueueBulk(integrityChecks);

	delete[] voxels;
}

bool PhysicsSystem::OverlapSphere(std::vector<BoxCollider*>& colliders, Vector3 center, float fRadius, uint32_t uiLayer /*= -1*/, bool queryTriggers /*= false*/) const
{
	Sphere overlapSphere(center, fRadius);

	bool overlapFound = false;
	for (BoxCollider* pBox : m_Colliders)
	{
		if ((pBox->GetLayer() & uiLayer) == 0) continue;
		if (!queryTriggers && pBox->IsTrigger()) continue;

		Box colliderBox(pBox);
		if (overlapSphere.Intersects(colliderBox))
		{
			colliders.push_back(pBox);
			overlapFound = true;
		}
	}

	return overlapFound;
}

bool PhysicsSystem::OverlapBox(std::vector<BoxCollider*>& colliders, Vector3 center, Vector3 extends, uint32_t uiLayer /*= -1*/, bool queryTriggers /*= false*/) const
{
	Box overlapBox;
	overlapBox.Min = center - extends;
	overlapBox.Max = center + extends;

	bool overlapFound = false;
	for (BoxCollider* pBox : m_Colliders)
	{
		if ((pBox->GetLayer() & uiLayer) == 0) continue;
		if (!queryTriggers && pBox->IsTrigger()) continue;

		Box colliderBox(pBox);
		if (overlapBox.Intersects(colliderBox))
		{
			colliders.push_back(pBox);
			overlapFound = true;
		}
	}

	return overlapFound;
}

bool sortbyfirst(const std::pair<float, BoxCollider*> &a,
	const std::pair<float, BoxCollider*> &b)
{
	return (a.first < b.first);
}


void PhysicsSystem::ResolveContinousCollision(const float deltaTime)
{
	for (BoxCollider* pColliderA : m_Colliders)
	{
		if (!pColliderA->IsEnabled()) continue;

		if (pColliderA->ContinuousCollision())
		{
			Manifold aManifold;
			if (CheckContinousCollision(pColliderA, aManifold, deltaTime))
			{
				if (aManifold.ShouldResolve)
				{
					auto transform = aManifold.Body1->GetTransform();
					if (transform)
					{
						if (!aManifold.Body2)
							aManifold.Body2 = m_pStaticBody;

						/* Impulse resolution */
						Vector3 relativeVel = aManifold.Body1->GetVelocity() - aManifold.Body2->GetVelocity();
						float contactVel = glm::dot(relativeVel, aManifold.Normal);

						if (contactVel <= -PHYSICS_EPSILON)
							continue;

						const float invMassSum = aManifold.Body1->GetInvMass() + aManifold.Body2->GetInvMass();
						if (invMassSum == 0) continue;

						contactVel /= invMassSum;

						Vector3 impulse = aManifold.Normal * contactVel;
						aManifold.Body1->ApplyImpulse(-impulse);
						aManifold.Body2->ApplyImpulse(impulse);

						// Adjust location
						Vector3 origin = transform->GetPosition();
						origin += aManifold.Normal * aManifold.Overlap;
						transform->SetPosition(origin);

					}

					aManifold.ShouldResolve = false;
				}
					

				m_Manifolds.push_back(aManifold);
			}
		}
	}
}


void PhysicsSystem::AccumulateManifolds()
{
	OPTICK_EVENT();
	m_Manifolds.clear();

	for (BoxCollider* pColliderA : m_Colliders)
	{
		if (!pColliderA->IsEnabled()) continue;

		for (BoxCollider* pColliderB : m_Colliders)
		{
			if (!pColliderB->IsEnabled()) continue;
			if (pColliderA == pColliderB) continue;
			if (pColliderA->GetOwner()->IsStatic()) continue;

			Manifold manifold;
			manifold.Collider1 = pColliderA;
			manifold.Body1 = pColliderA->GetOwner()->GetComponent<PhysicsBody>();
			manifold.Collider2 = pColliderB;
			manifold.ShouldResolve = !pColliderA->IsTrigger() && !pColliderB->IsTrigger();

			Box boxA(pColliderA);
			Box boxB(pColliderB);
			if (boxA.Intersects(boxB, manifold))
			{
				manifold.Body2 = pColliderB->GetOwner()->GetComponent<PhysicsBody>();
				m_Manifolds.push_back(manifold);
			}
		}
	}
}

void PhysicsSystem::ResolveManifolds()
{
	OPTICK_EVENT();
	for (Manifold& manifold : m_Manifolds)
	{
		PhysicsBody* pBody1 = manifold.Body1;
		PhysicsBody* pBody2 = manifold.Body2;
		if (!manifold.Collider1->IgnoreVoxels() && !manifold.Collider1->ContinuousVoxelCollision() && 
			!pBody2 && manifold.Collider2->GetOwner()->IsStatic() && manifold.Collider2->VoxelPreciseCollision())
		{
			SolveVoxelPreciseCollision(manifold.Collider1, manifold.Collider2);
			continue;
		}
		else if (manifold.ShouldResolve && pBody1)
		{
			if (!pBody2)
				pBody2 = m_pStaticBody;

			/* Impulse resolution */
			Vector3 relativeVel = pBody1->GetVelocity() - pBody2->GetVelocity();
			float contactVel = glm::dot(relativeVel, manifold.Normal);

			if (contactVel >= -PHYSICS_EPSILON)
				continue;

			const float invMassSum = pBody1->GetInvMass() + pBody2->GetInvMass();
			if (invMassSum == 0) continue;

			contactVel /= invMassSum;

			Vector3 impulse = manifold.Normal * contactVel;
			pBody1->ApplyImpulse(-impulse);
			pBody2->ApplyImpulse(impulse);

			/* Positional correction */
			Vector3 worldPos = manifold.Collider1->GetTransform()->GetPosition();
			worldPos += manifold.Normal * manifold.Overlap;

			/* Small check to keep inside grid bounds */
			if (pBody1->GetCollider())
				ClampToBounds(worldPos, manifold.Collider1);

			pBody1->GetTransform()->SetPosition(worldPos);

			Vector3 gridPos = m_VoxelGrid.WorldToGrid(worldPos);
			manifold.Collider1->SetGridPosition(gridPos);
		}
	}

	// Handle voxel precise collision for colliders with the Continuous Voxel Collision option on
	// Warning voxel precise collision can impact performance if many colliders have it active
	//for (BoxCollider* pCollider : m_Colliders)
	//{
	//	if (pCollider->ContinuousVoxelCollision() && !pCollider->IgnoreVoxels() && !pCollider->GetOwner()->IsStatic())
	//	{
	//		SolveVoxelPreciseCollision(pCollider);
	//	}
	//}
}

void PhysicsSystem::SolveVoxelPreciseCollision(Collider* pColliderA, const Collider* pColliderB)
{
	OPTICK_EVENT();
	BoxCollider* pBoxColliderA = static_cast<BoxCollider*>(pColliderA);
	const BoxCollider* pBoxColliderB = static_cast<const BoxCollider*>(pColliderB);

	Vector3 max = glm::min(pBoxColliderA->GetBoxMax(), pBoxColliderB->GetBoxMax());
	Vector3 min = glm::max(pBoxColliderA->GetBoxMin(), pBoxColliderB->GetBoxMin());

	min = glm::min(min, max);
	max = glm::max(min, max);

#if defined(EDITOR) || defined(_DEBUG)
	{
		auto debugRenderer = m_pWorld->GetDebugRenderer();
		DebugBox box;
		box.m_Color = VColors::BlueViolet;
		box.m_Extents = (max - min) * 0.5f;
		box.m_Center = min + box.m_Extents;
		debugRenderer->AddBox(box);
	}
#endif

	Vector3 colliderGridPos = m_VoxelGrid.WorldToGrid(min, true);
	UVector3 chunkSize = static_cast<UVector3>(max - min);
	uint32_t numVoxels = chunkSize.x * chunkSize.y * chunkSize.z;
	Voxel** voxels = new Voxel*[numVoxels];
	m_VoxelGrid.GetChunk(voxels, colliderGridPos, chunkSize, true);
		
	bool isHandled = false;
	pColliderA->OnVoxelCollision(voxels, numVoxels, isHandled);

	if (!isHandled)
	{
		bool isColliding = false;

		/* Collision check that makes a bounding box around the active voxels it encounters
		   Simple Box to Box intersection is used after that */
		Vector3 voxelBoxMin = chunkSize;
		Vector3 voxelBoxMax(0, 0, 0);

		for (uint32_t i = 0; i < numVoxels; ++i)
		{
			if (!voxels[i] || !voxels[i]->Active) continue;

			Vector3 voxelPos(
				i % chunkSize.x,
				(i / chunkSize.x) % chunkSize.y,
				i / (chunkSize.x * chunkSize.y)
			);

			if (voxelBoxMin.x > voxelPos.x) voxelBoxMin.x = voxelPos.x;
			if (voxelBoxMin.y > voxelPos.y) voxelBoxMin.y = voxelPos.y;
			if (voxelBoxMin.z > voxelPos.z) voxelBoxMin.z = voxelPos.z;

			if (voxelBoxMax.x < voxelPos.x + 1) voxelBoxMax.x = voxelPos.x + 1;
			if (voxelBoxMax.y < voxelPos.y + 1) voxelBoxMax.y = voxelPos.y + 1;
			if (voxelBoxMax.z < voxelPos.z + 1) voxelBoxMax.z = voxelPos.z + 1;

			isColliding = true;
		}

		if (isColliding)
		{
			Box A, B;
			A.Min = m_VoxelGrid.WorldToGrid(pBoxColliderA->GetBoxMin(), true);
			A.Max = m_VoxelGrid.WorldToGrid(pBoxColliderA->GetBoxMax(), true);
			B.Max = colliderGridPos + voxelBoxMax;
			B.Min = colliderGridPos + voxelBoxMin;

			Manifold manifold;
			if (A.Intersects(B, manifold))
			{
				Vector3 newPos = glm::floor(pColliderA->GetTransform()->GetPosition() + manifold.Normal * manifold.Overlap);
				pColliderA->GetTransform()->SetPosition(newPos);
			}
		}
	}

	delete[] voxels;
}

void PhysicsSystem::HandleCallbacks()
{
	OPTICK_EVENT();
	for (Collider* pCollider : m_Colliders)
		pCollider->ResetCollisions();

	for (Manifold& manifold : m_Manifolds)
	{
		manifold.Collider1->HandleCollision(manifold);
	}

	for (Collider* pCollider : m_Colliders)
		pCollider->CleanCollisions();
}

void PhysicsSystem::StepCheck(PhysicsBody* pBody, const VoxFrame* pFrame /*= nullptr*/)
{
	uint32_t dimensionX, dimensionY, dimensionZ;
	m_VoxelGrid.GetDimensions(dimensionX, dimensionY, dimensionZ);

	BoxCollider* pCollider = pBody->GetCollider();
	UVector3 boxSize = static_cast<UVector3>(pCollider->GetBoxSize());
	Vector3 chunkStart = m_VoxelGrid.WorldToGrid(pCollider->GetBoxMin());

	/* return if the chunk start is already out of bounds */
	if (chunkStart.y >= dimensionY) 
		return;

	Voxel** voxels = new Voxel*[static_cast<size_t>(boxSize.x * boxSize.z)];
	bool valid = m_VoxelGrid.GetChunk(voxels, chunkStart, Vector3(boxSize.x, 1, boxSize.z), true);
	if (!valid)
	{
		delete[] voxels;
		return;
	}

	for (uint32_t x = 0; x < boxSize.x; ++x)
	{
		for (uint32_t z = 0; z < boxSize.z; ++z)
		{
			uint32_t chunkId = static_cast<uint32_t>(x + boxSize.x * z);

			/* Skip if the voxel is not active or invalid */
			if (!voxels[chunkId] || !voxels[chunkId]->Active) continue;

			/* Skip if a voxel above the maximum step height is active or invalid */
			const Voxel* pVoxel = m_VoxelGrid.GetVoxel(
				static_cast<int>(chunkStart.x + x),
				static_cast<int>(chunkStart.y + pBody->GetStepHeight()),
				static_cast<int>(chunkStart.z + z)
			);

			if (!pVoxel || pVoxel->Active) break;

			uint32_t yHeight = 0;
			for (uint32_t i = 0; i <= pBody->GetStepHeight(); ++i)
			{
				const Voxel* pStepVoxel = m_VoxelGrid.GetVoxel(
					static_cast<int>(chunkStart.x + x),
					static_cast<int>(chunkStart.y + i),
					static_cast<int>(chunkStart.z + z)
				);

				if (pStepVoxel && pStepVoxel->Active)
					yHeight = i + 1;
				else break;
			}

			Vector3 pos = pBody->GetTransform()->GetPosition();
			pos.y += yHeight;
			pBody->GetTransform()->SetPosition(pos);

			/* Reset y velocity on step */
			Vector3 vel = pBody->GetVelocity();
			vel.y = 0;

			pBody->SetVelocity(vel);
			pBody->SetResting(true);

			delete[] voxels;
			return;
		}
	}

	// Check voxels below the collider to make sure it can still stay in resting state
	if (pBody->IsResting())
	{
		pBody->SetResting(false);

		uint32_t arrSize = static_cast<uint32_t>(boxSize.x * boxSize.z);
		Voxel** voxelBelow = new Voxel*[arrSize];
		chunkStart.y -= 1;
		if (m_VoxelGrid.GetChunk(voxelBelow, chunkStart, Vector3(boxSize.x, 1, boxSize.z), true))
		{
			for (uint32_t i = 0; i < arrSize; ++i)
			{
				if (voxelBelow[i] && voxelBelow[i]->Active)
				{
					pBody->SetResting(true);
					break;
				}
			}
		}

		delete[] voxelBelow;
	}
	
	delete[] voxels;
}

bool PhysicsSystem::ClampToBounds(Vector3& position, Collider* pCollider)
{
	BoxCollider* pBoxCollider = static_cast<BoxCollider*>(pCollider);
	Vector3 vel = Vector3(0.f);
	PhysicsBody* pBody = pCollider->GetOwner()->GetComponent<PhysicsBody>();
	if (pBody)
		vel = pBody->GetVelocity();

	Vector3 boxBounds = pBoxCollider->GetHalfBoxSize();

	uint32_t dimensionX, dimensionY, dimensionZ;
	m_VoxelGrid.GetDimensions(dimensionX, dimensionY, dimensionZ);
	dimensionX = m_pWorld->GetWorldSize().x;
	dimensionZ = m_pWorld->GetWorldSize().y;
	
	bool clamped = false;

	if (position.x - boxBounds.x < 1)
	{
		clamped = true;
		position.x = boxBounds.x + 1;
		vel.x = 0;
	}
	else if (position.x + boxBounds.x > dimensionX - 1)
	{
		clamped = true;
		position.x = dimensionX - boxBounds.x - 1;
		vel.x = 0;
	}

	if (position.y - boxBounds.y < 1)
	{
		clamped = true;
		position.y = boxBounds.y + 1;
		vel.y = 0;

		if (pBody)
			pBody->SetResting(true);
		
	}
	else if (position.y + boxBounds.y > dimensionY - 1)
	{
		clamped = true;
		position.y = dimensionY - boxBounds.y - 1;
		vel.y = 0;
	}

	if (position.z - boxBounds.z < 1)
	{
		clamped = true;
		position.z = boxBounds.z + 1;
		vel.z = 0;
	}
	else if (position.z + boxBounds.z > dimensionZ - 1)
	{
		clamped = true;
		position.z = dimensionZ - boxBounds.z - 1;
		vel.z = 0;
	}	

	if (pBody)
	{
		pBody->SetVelocity(vel);
	}		

	return clamped;
}

bool PhysicsSystem::IntersectRayAABB(const float* invRayDir, const float* rayStart, const float* boxMin, const float* boxMax, float& tNear)
{
	float tNearLocal = -FLT_MAX;
	float tFarLocal = FLT_MAX;

	for (uint32_t i = 0; i < 3; i++)
	{
 		if (invRayDir[i] == 0)
		{
			if (rayStart[i] < boxMin[i] || rayStart[i] > boxMax[i])
				return false;
		}
		else
		{
			float tMin = (boxMin[i] - rayStart[i]) * invRayDir[i];
			float tMax = (boxMax[i] - rayStart[i]) * invRayDir[i];

			// Make sure tMin is always minimum value
			if (tMin > tMax)
			{
				float temp = tMin;
				tMin = tMax;
				tMax = temp;
			}

			if (tMin > tNearLocal)
				tNearLocal = tMin;

			if (tMax < tFarLocal)
				tFarLocal = tMax;

			if (tNearLocal > tFarLocal || tFarLocal < 0)
				return false;
		}
	}

	if (tNearLocal < tNear && tNearLocal > 0)
		tNear = tNearLocal;
	else if (tFarLocal < tNear)
		tNear = tFarLocal;

	return true;
}

bool PhysicsSystem::CheckContinousCollision(BoxCollider* pColliderA, Manifold& manifold, float deltaTime)
{
	PhysicsBody* pPhysBodyA = pColliderA->GetOwner()->GetComponent<PhysicsBody>();
	if (!pPhysBodyA)
		return false;

	manifold.Body1 = pPhysBodyA;
	manifold.Collider1 = pColliderA;

	Transform* transform = pColliderA->GetTransform();
	Vector3 centerA = transform->GetPosition();
	Vector3 extends = pPhysBodyA->GetVelocity() * deltaTime;

	if( (std::abs(extends.x) + std::abs(extends.y) + std::abs(extends.z)) < 5.f)
		return false;

	float lenght = glm::length(extends);
	manifold.Normal = normalize(extends);

	Vector3 halfSizeA = pColliderA->GetHalfBoxSize();

	HitResult hitRes;

	std::vector<BoxCollider*> potentialColliders;

	Vector3 OverlapBoxMin = glm::min(pColliderA->GetBoxMin(), pColliderA->GetBoxMin() + extends);
	Vector3 OverlapBoxMax = glm::max(pColliderA->GetBoxMax(), pColliderA->GetBoxMax() + extends);

	Vector3 OverlapBoxExtends = OverlapBoxMax - OverlapBoxMin;

	// Check for potential colliders
	if (!OverlapBox(potentialColliders, OverlapBoxMin, glm::abs(OverlapBoxExtends), pColliderA->GetLayer(), true) || potentialColliders.size() < 2u)
		return false;

	float dot = FLT_MAX;
	Vector3 origin;

	for (float z = -halfSizeA.z; z <= halfSizeA.z; z += halfSizeA.z * 2.f)
	{
		for (float y = -halfSizeA.y; y <= halfSizeA.y; y += halfSizeA.y * 2.f)
		{
			for (float x = -halfSizeA.x; x <= halfSizeA.x; x += halfSizeA.x * 2.f)
			{
				origin = { x,y,z };

#if defined(EDITOR) || defined(_DEBUG)							
					{
						auto debugRenderer = m_pWorld->GetDebugRenderer();
						{
							DebugLine line;
							line.m_Start = centerA + origin;
							line.m_End = centerA + origin + manifold.Normal * (lenght * 2.f);
							line.m_Color = VColors::Blue;
							debugRenderer->AddLine(line);
						}
					}
#endif
				Vector3 rayStart = centerA;
				BoxCollider* pIgnoreCollider = pColliderA;
				float rayLenght = lenght;

				while( rayLenght > 0.f && RayCastGroup(pIgnoreCollider, potentialColliders, rayStart + origin, manifold.Normal, hitRes, rayLenght))
				{
					if (hitRes.HitEntity)
					{
						BoxCollider* pColliderB = hitRes.HitEntity->GetComponent<BoxCollider>();
						if (pColliderB)
						{
							if (!pColliderB->VoxelPreciseCollision() && pColliderB->IsEnabled(true))
							{
								float newDot = glm::dot(hitRes.HitPoint - (rayStart - origin), manifold.Normal);

								if (newDot < dot)
								{
									dot = newDot;
								}
							}
							else
							{
								float dist = glm::distance(hitRes.HitPoint, rayStart);
								if (dist == 0.f)
									break;

								rayLenght -= dist;
								rayStart = hitRes.HitPoint;
								pIgnoreCollider = pColliderB;		
								continue;
							}
						}
					}
					break;
				}
			}
		}
	}

	if(!hitRes.HitEntity || dot > lenght)
		return false;

	BoxCollider* pColliderB = hitRes.HitEntity->GetComponent<BoxCollider>();

#if defined(EDITOR) || defined(_DEBUG)							
	{

		Vector3 origin = centerA + manifold.Normal * (dot - PHYSICS_MAGICAL_EVERYTHING_SOLVING_VALUE);

		auto debugRenderer = m_pWorld->GetDebugRenderer();
		{
			DebugLine line;
			line.m_Start = centerA;
			line.m_End = origin;
			line.m_Color = VColors::Yellow;
			debugRenderer->AddLine(line);
		}

		{
			DebugBox box;
			box.m_Center = centerA + extends;
			box.m_Extents = glm::abs(extends) + pColliderA->GetHalfBoxSize();
			box.m_Color = VColors::LimeGreen;
			debugRenderer->AddBox(box);
		}

		{
			DebugSphere sphere;
			sphere.m_Center = origin;
			sphere.m_fRadius = 5.f;
			sphere.m_Color = VColors::Aqua;
			debugRenderer->AddSphere(sphere);
		}

		{
			DebugBox box;
			box.m_Center = origin;
			box.m_Extents = pColliderA->GetHalfBoxSize();
			box.m_Color = VColors::Yellow;
			debugRenderer->AddBox(box);
		}


	}

#endif

	manifold.Overlap = dot - PHYSICS_MAGICAL_EVERYTHING_SOLVING_VALUE;
	manifold.Body2 = pColliderB->GetOwner()->GetComponent<PhysicsBody>();
	manifold.Collider2 = pColliderB;
	manifold.ShouldResolve = !pColliderA->IsTrigger() && !pColliderB->IsTrigger();

	return true;
}

void PhysicsSystem::SimulateParticles(float fDeltaTime)
{
	OPTICK_EVENT();
	Particle* aliveParticle = m_ParticlePool.GetLastAlive();
	
	GPUParticle* pGPUParticles = reinterpret_cast<GPUParticle*>(m_pGPUParticles->GetData());
	GPUParticle gpuParticle;

	while (aliveParticle != nullptr)
	{
		if (m_uiActiveParticleCount >= m_uiMaxParticleCount)
			break;

		++m_uiActiveParticleCount;

		// Simulate CPU particle
		Particle* nextParticle = aliveParticle->Prev;

		// Handle particle timer
		if (aliveParticle->Live.Timer > 0.f)
		{
			if (UpdateParticleTimer(aliveParticle, fDeltaTime))
			{
				aliveParticle = nextParticle;
				continue;
			}
		}

		Vector3& prevGridPos = aliveParticle->Live.GridPosition;
		Vector3 newGridPos = m_VoxelGrid.WorldToGrid(aliveParticle->Live.Position, true);

		//Clamp previous grid position when its distance exceeds 100 units
		if (glm::distance2(newGridPos, prevGridPos) > 10000)
		{
			prevGridPos = newGridPos;
			aliveParticle->Live.GridPosition = prevGridPos;
		}

		aliveParticle->Live.Velocity += PARTICLE_GRAVITY * fDeltaTime;
		aliveParticle->Live.Position += aliveParticle->Live.Velocity * fDeltaTime;

		newGridPos = m_VoxelGrid.WorldToGrid(aliveParticle->Live.Position, true);

		// Clamp y to zero to avoid particles being destroyed by to high velocities
		if (newGridPos.y < 0.f)
			newGridPos.y = 0.f;

		// Update the particles position if its positions has changed
		if (prevGridPos != newGridPos)
		{
			Voxel* pVoxel = m_VoxelGrid.GetVoxel(
				static_cast<int>(newGridPos.x),
				static_cast<int>(newGridPos.y),
				static_cast<int>(newGridPos.z)
			);

			if (pVoxel && pVoxel->Active)
			{
				float speed = glm::length(aliveParticle->Live.Velocity);

				Vector3 normal = glm::normalize(prevGridPos - newGridPos);
				Vector3 particleVelocity = glm::normalize(aliveParticle->Live.Velocity);

				// Bake particle and destroy if velocity is to low or the particle is going straight down
				if (speed < PARTICLE_DESTROY_THRESHOLD || normal == Vector3(0, 1, 0) || prevGridPos.y < 0) //particleVelocity.y + 1.f <= 0.001f
				{
					Voxel* pOldVoxel = m_VoxelGrid.GetVoxel(
						static_cast<int>(prevGridPos.x),
						static_cast<int>(prevGridPos.y),
						static_cast<int>(prevGridPos.z)
					);

					// Only handle particle if its old voxel is still valid
					if (pOldVoxel && (pOldVoxel->UserPointer == (uintptr_t)aliveParticle || !pOldVoxel->UserPointer))
					{
						/* Set previous voxel to default state if the particle shouldn't bake */
						if (!aliveParticle->Live.BakeOnImpact)
						{
							pOldVoxel->UserPointer = 0;
						}
						/* Bake the particle into the grid */
						else if (aliveParticle->Live.BakeOnImpact)
						{
							pOldVoxel->UserPointer = 0;

							Vector3 bakeVoxelPos = newGridPos;
							bakeVoxelPos.y += 1;

							Voxel* pBakeVoxel = m_VoxelGrid.GetVoxel(
								static_cast<int>(bakeVoxelPos.x),
								static_cast<int>(bakeVoxelPos.y),
								static_cast<int>(bakeVoxelPos.z)
							);

							if (pBakeVoxel && pBakeVoxel->Active)
							{
								if (bakeVoxelPos.y > 1) 
									bakeVoxelPos.y -= 1;
								pBakeVoxel = FindEmtpyNeighbor(bakeVoxelPos);
							}

							if (pBakeVoxel && !pBakeVoxel->Active)
							{
								m_VoxelGrid.ModifyVoxel(
									(int)bakeVoxelPos.x,
									(int)bakeVoxelPos.y,
									(int)bakeVoxelPos.z,
									aliveParticle->Live.VoxelColor.inst.Color
								);
								m_pRenderSystem->ModifyVoxel(
									(uint32_t)bakeVoxelPos.x,
									(uint32_t)bakeVoxelPos.y,
									(uint32_t)bakeVoxelPos.z,
									aliveParticle->Live.VoxelColor.inst.Color
								);

								pBakeVoxel->Active = true;
								pBakeVoxel->UserPointer = (uintptr_t)aliveParticle->Live.UserPointer;
							}
						}
					}
					m_ParticlePool.DestroyParticle(aliveParticle);
				}
				else
				{
					/* Apply simple bounce physics to particle */
					float contactVel = glm::dot(aliveParticle->Live.Velocity, normal);
					if (contactVel < 0)
					{
						aliveParticle->Live.Velocity += -normal * contactVel * PARTICLE_BOUNCE_MULIPLIER;
					}
				}
			}
			else if (pVoxel && !pVoxel->Active)
			{
				Voxel* pOldVoxel = m_VoxelGrid.GetVoxel(
					static_cast<int>(prevGridPos.x),
					static_cast<int>(prevGridPos.y),
					static_cast<int>(prevGridPos.z)
				);

				if (pOldVoxel && (pOldVoxel->UserPointer == (uintptr_t)aliveParticle || !pOldVoxel->UserPointer))
				{
					pOldVoxel->UserPointer = 0;
				}

				/* Fill and Clear voxels based on the new grid position */
				aliveParticle->Live.GridPosition = newGridPos;
				pVoxel->UserPointer = (uintptr_t)aliveParticle;
			}
			else
			{
				/* Clear voxels that reach out of bounds */
				Voxel* pOldVoxel = m_VoxelGrid.GetVoxel(
					static_cast<int>(prevGridPos.x),
					static_cast<int>(prevGridPos.y),
					static_cast<int>(prevGridPos.z)
				);

				if (pOldVoxel && (pOldVoxel->UserPointer == (uintptr_t)aliveParticle || !pOldVoxel->UserPointer))
				{
					pOldVoxel->UserPointer = 0;
				}
				m_ParticlePool.DestroyParticle(aliveParticle);
			}
		}

		// Create GPU particle
		gpuParticle.Position = aliveParticle->Live.Position;
		gpuParticle.VoxelColor = aliveParticle->Live.VoxelColor;

		std::memcpy(
			&pGPUParticles[m_uiActiveParticleCount - 1],
			&gpuParticle,
			sizeof(GPUParticle)
		);

		// Iterate to next particle
		aliveParticle = nextParticle;
	}
}

bool PhysicsSystem::UpdateParticleTimer(Particle* pParticle, float fDeltaTime)
{
	pParticle->Live.Timer -= fDeltaTime;
	if (pParticle->Live.Timer <= 0.f)
	{
		Vector3 prevGridPos = pParticle->Live.GridPosition;
		Voxel* pOldVoxel = m_VoxelGrid.GetVoxel(
			static_cast<int>(prevGridPos.x),
			static_cast<int>(prevGridPos.y),
			static_cast<int>(prevGridPos.z)
		);

		if (pOldVoxel && (pOldVoxel->UserPointer == (uintptr_t)pParticle || !pOldVoxel->UserPointer))
		{
			pOldVoxel->UserPointer = 0;
		}

		m_ParticlePool.DestroyParticle(pParticle);
		return true;
	}
	return false;
}

Voxel* PhysicsSystem::FindEmtpyNeighbor(Vector3 gridPos, uint32_t ySearchCount)
{
	for (int y = 0; y < static_cast<int>(ySearchCount); ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			for (int z = -1; z <= 1; ++z)
			{
				if (x == 0 && z == 0) continue;

				Vector3 bakeVoxelPos = gridPos + Vector3(
					static_cast<float>(x),
					static_cast<float>(y),
					static_cast<float>(z));

				Voxel* pBakeVoxel = m_VoxelGrid.GetVoxel(
					static_cast<int>(bakeVoxelPos.x),
					static_cast<int>(bakeVoxelPos.y),
					static_cast<int>(bakeVoxelPos.z)
				);

				if (pBakeVoxel && !pBakeVoxel->Active)
					return pBakeVoxel;
			}
		}
	}

	return nullptr;
}


