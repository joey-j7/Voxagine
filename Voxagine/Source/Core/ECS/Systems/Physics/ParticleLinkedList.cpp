#include "pch.h"
#include "ParticleLinkedList.h"

#include "Core/Platform/Rendering/Objects/Mapper.h"

ParticleLinkedList::ParticleLinkedList(uint32_t uiReserveSize)
{
	m_pFirstAliveParticle = nullptr;
	m_pLastAliveParticle = nullptr;

	/* Add new particles */
	m_Particles.reserve(uiReserveSize);
	for (uint32_t i = 0; i < uiReserveSize; ++i)
		m_Particles.push_back(Particle());

	m_pAvailableParticle = &m_Particles[0];

	/* Each particle points to the next */
	for (uint32_t i = 0; i < uiReserveSize - 1; ++i)
		m_Particles[i].NextAvailable = &m_Particles[i + 1];

	/* The last one terminates the list */
	m_Particles[uiReserveSize - 1].NextAvailable = nullptr;
}

Particle* ParticleLinkedList::SpawnParticle()
{
	if (!m_pAvailableParticle)
		return nullptr;

	Particle* particle = m_pAvailableParticle;
	m_pAvailableParticle = particle->NextAvailable;

	InitParticle(particle);
	
	if (!m_pFirstAliveParticle)
	{
		m_pFirstAliveParticle = particle;
		m_pLastAliveParticle = particle;
	}
	else
	{
		m_pFirstAliveParticle->Prev = particle;
		particle->Next = m_pFirstAliveParticle;
		m_pFirstAliveParticle = particle;
	}

	return particle;
}

void ParticleLinkedList::DestroyParticle(Particle* pParticle)
{
	pParticle->NextAvailable = m_pAvailableParticle;
	m_pAvailableParticle = pParticle;

	if (pParticle->Next && pParticle->Prev)
	{
		pParticle->Prev->Next = pParticle->Next;
		pParticle->Next->Prev = pParticle->Prev;
	}
	else if (pParticle->Next)
	{
		pParticle->Next->Prev = nullptr;
		m_pFirstAliveParticle = pParticle->Next;
	}
	else if (pParticle->Prev)
	{
		pParticle->Prev->Next = nullptr;
		m_pLastAliveParticle = pParticle->Prev;
	}
	else
	{
		m_pFirstAliveParticle = nullptr;
		m_pLastAliveParticle = nullptr;
	}
	
	pParticle->Next = nullptr;
	pParticle->Prev = nullptr;
}

void ParticleLinkedList::InitParticle(Particle* pParticle)
{
	pParticle->Live.Velocity = Vector3(0.f);
	pParticle->Live.Position = Vector3(0.f);
	pParticle->Live.GridPosition = Vector3(0.f);
	pParticle->Live.VoxelColor = 0;
	pParticle->Live.BakeOnImpact = true;
	pParticle->Live.UserPointer = nullptr;
	pParticle->Live.Timer = NO_PARTICLE_TIMER;
}
