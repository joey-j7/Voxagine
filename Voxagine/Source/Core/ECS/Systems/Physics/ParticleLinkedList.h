#pragma once
#include "Core/ECS/Systems/Physics/VoxelGrid.h"

#include "Core/Math.h"

#define NO_PARTICLE_TIMER -1.f

class Particle
{
	friend class ParticleLinkedList;

private:
	Particle() {}

public:
	Particle* Next = nullptr;
	Particle* Prev = nullptr;

	union
	{
		// State when it's in use.
		struct
		{
			void* UserPointer = nullptr;
			Vector3 Velocity = Vector3(0.f);
			Vector3 Position = Vector3(0.f);
			Vector3 GridPosition = Vector3(0.f);
			VColor VoxelColor = 0;

			bool BakeOnImpact = true;
			float Timer = NO_PARTICLE_TIMER;
		} Live;

		// State when it's available.
		Particle* NextAvailable;

	};
};

struct GPUParticle
{
	Vector3 Position = Vector3(0.f);
	VColor VoxelColor = 0;
};

static_assert((sizeof(GPUParticle) % 16) == 0, "Particle not padded correctly");

class ParticleLinkedList
{
	friend class RenderSystem;

public:
	ParticleLinkedList(uint32_t uiReserveSize);

	Particle* SpawnParticle();
	void DestroyParticle(Particle* pParticle);
	Particle* GetFirstAlive() { return m_pFirstAliveParticle; }
	Particle* GetLastAlive() { return m_pLastAliveParticle; }

	uint32_t GetSize() { return (uint32_t)m_Particles.size(); }

private:
	void InitParticle(Particle* pParticle);

	Particle* m_pAvailableParticle;
	Particle* m_pFirstAliveParticle;
	Particle* m_pLastAliveParticle;

	std::vector<Particle> m_Particles;
};