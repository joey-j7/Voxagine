#pragma once
#include <unordered_map>
#include "Core/ECS/ComponentSystem.h"
#include "Core/ECS/Systems/Physics/ParticleLinkedList.h"
#include "Core/ECS/Systems/Chunk/Chunk.h"

#define PHYSICS_EPSILON 0.000797

class PhysicsBody;
class BoxCollider;
struct VoxFrame;
struct Manifold;
struct HitResult;
class Box;
class RenderSystem;
class ParticleSystem;
class IntegrityJob;
class Mapper;

class PhysicsSystem : public ComponentSystem
{
public:
	friend class RenderSystem;
	friend class VoxelBaker;

	PhysicsSystem(World* pWorld, Vector3 gridSize, uint32_t voxelSize, uint32_t uiMaxParticles = 150000, UVector3 chunkSize = DEFAULT_CHUNK_SIZE);
	virtual ~PhysicsSystem();

	virtual void Start() override;

	virtual bool CanProcessComponent(Component* pComponent) override;

	virtual void FixedTick(const GameTimer& fixedTimer) override;
	virtual void PostTick(float fDeltaTime) override;
	
	// Performs a ray intersection test against all collider's in the scene
	// Takes a start point and a direction with a specific length
	// Specific collision layers can be tested against, default is all layers
	bool RayCast(Vector3 start, Vector3 dir, HitResult& hitResult, float fLength = FLT_MAX, uint32_t uiLayer = -1);
	bool RayCastSingle(const BoxCollider* pCollider, Vector3 start, Vector3 dir, HitResult& hitResult, float fLength = FLT_MAX, uint32_t uiLayer = -1);

	bool RayCastGroup(const BoxCollider* pIgnoreCollider, std::vector<BoxCollider*>& colliders, Vector3 start, Vector3 dir, HitResult& hitResult, float fLength = FLT_MAX, uint32_t uiLayer = -1);
	inline bool RayCastGroup(std::vector<BoxCollider*>& colliders, Vector3 start, Vector3 dir, HitResult& hitResult, float fLength = FLT_MAX, uint32_t uiLayer = -1);
	
	// Creates particles of static voxels in the radius of the sphere with a force applied from the center of the sphere
	void ApplySphericalDestruction(const Vector3& position, float fRadius, float fForceMin, float fForceMax, bool bBakeParticles = true);

	// Queries all colliders in the world against a given sphere
	// Fills the colliders vector with intersected colliders
	// Returns true if any collider was intersected
	bool OverlapSphere(std::vector<BoxCollider*>& colliders, Vector3 center, float fRadius, uint32_t uiLayer = -1, bool queryTriggers = false) const;

	// Queries all colliders in the world against a given bounding box
	// Fills the colliders vector with intersected colliders
	// Returns true if any collider was intersected
	bool OverlapBox(std::vector<BoxCollider*>& colliders, Vector3 center, Vector3 extends, uint32_t uiLayer = -1, bool queryTriggers = false) const;

	VoxelGrid* GetVoxelGrid() { return &m_VoxelGrid; }
	uint32_t GetParticlePoolSize() { return m_ParticlePool.GetSize(); }

	Mapper* GetGPUParticles() const { return m_pGPUParticles; }

	void SetRenderSystem(RenderSystem* pRenderSystem) { m_pRenderSystem = pRenderSystem; }

	uint32_t m_uiActiveParticleCount = 0;

	static const Vector3 PARTICLE_GRAVITY;

protected:
	virtual void OnComponentAdded(Component* pComponent) override;
	virtual void OnComponentDestroyed(Component* pComponent) override;

	void OnWorldPaused(World* pWorld);
	void OnWorldResumed(World* pWorld);
	void OnIntegrityJobStopped(IntegrityJob* pJob);

	void TickBodies(const GameTimer& fixedTimer);
	void TickParticleSystems(const GameTimer& fixedTimer);
	void SyncIntegrityJob();

	void ResolveContinousCollision(const float deltaTime);
	void AccumulateManifolds();
	void ResolveManifolds();

	void SolveVoxelPreciseCollision(Collider* pColliderA, const Collider* pColliderB);

	void HandleCallbacks();

	//Performs StepHeight check for physics bodies. Default StepCheck will be performed using the attached collider,
	//the function can be supplied with VoxModel in which case a voxel perfect check will be performed.
	//Voxel perfect StepHeight is currently not supported
	void StepCheck(PhysicsBody* pBody, const VoxFrame* pFrame = nullptr);

	bool ClampToBounds(Vector3& position, Collider* pCollider);
	bool IntersectRayAABB(const float* invRayDir, const float* rayStart, const float* boxMin, const float* boxMax, float& tNear);


	bool CheckContinousCollision(BoxCollider* pColliderA, Manifold& manifold, float deltaTime);

protected:
	// Particle simulation functions
	void SimulateParticles(float fDeltaTime);

	// Updates given particle timer and destroys the particle once the timer has finished
	// Returns true if the timer has finished
	bool UpdateParticleTimer(Particle* pParticle, float fDeltaTime);

	// Searched an active voxel in the voxel grid around the given gridpos
	// ySearchCount = the maximum number of y positions it will search above gridpos
	Voxel* FindEmtpyNeighbor(Vector3 gridPos, uint32_t ySearchCount = 2);

private:
	VoxelGrid m_VoxelGrid;
	RenderSystem* m_pRenderSystem;
	IntegrityJob* m_pIntegrityJob;

	Entity* m_pStaticEntityBody;
	PhysicsBody* m_pStaticBody;

	uint32_t m_uiMaxParticleCount = 0;
	Mapper* m_pGPUParticles = nullptr;
	ParticleLinkedList m_ParticlePool;

	std::vector<ParticleSystem*> m_ParticleSystems;
	std::vector<PhysicsBody*> m_Bodies;
	std::vector<BoxCollider*> m_Colliders;
	std::vector<Manifold> m_Manifolds;

	static const float PARTICLE_DESTROY_THRESHOLD;
	static const float PARTICLE_BOUNCE_MULIPLIER;
};