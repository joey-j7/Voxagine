#include "pch.h"
#include "ParticlePool.h"

ParticlePool::ParticlePool()
{
}

ParticlePool::~ParticlePool()
{
	delete[] Position;
	delete[] GridPosition;
	delete[] Velocity;
	delete[] Color;
	delete[] Timer;
	delete[] Alive;
}

void ParticlePool::Create(uint32_t uiNumParticles)
{
	m_uiNumParticles = uiNumParticles;
	m_uiNumParticlesAlive = 0;

	Position = new Vector3[uiNumParticles];
	GridPosition = new Vector3[uiNumParticles];
	Velocity = new Vector3[uiNumParticles];
	Color = new VColor[uiNumParticles];
	Timer = new float[uiNumParticles];
	Alive = new bool[uiNumParticles];
}

void ParticlePool::SpawnParticle(uint32_t uiIndex)
{
	if (m_uiNumParticlesAlive < m_uiNumParticles)
	{
		Alive[uiIndex] = true;
		SwapParticle(uiIndex, m_uiNumParticlesAlive);
		m_uiNumParticlesAlive++;
	}
}

void ParticlePool::DestroyParticle(uint32_t uiIndex)
{
	if (m_uiNumParticlesAlive > 0)
	{
		Alive[uiIndex] = false;
		SwapParticle(uiIndex, m_uiNumParticlesAlive - 1);
		m_uiNumParticlesAlive--;
	}
}

void ParticlePool::SwapParticle(uint32_t a, uint32_t b)
{
	std::swap(Position[a], Position[b]);
	std::swap(GridPosition[a], GridPosition[b]);
	std::swap(Velocity[a], Velocity[b]);
	std::swap(Color[a], Color[b]);
	std::swap(Timer[a], Timer[b]);
	std::swap(Alive[a], Alive[b]);
}
