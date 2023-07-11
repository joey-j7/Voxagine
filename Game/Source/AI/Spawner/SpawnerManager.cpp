#include "SpawnerManager.h"

#include "Core/ECS/World.h"
#include "Core/ECS/Entity.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "AI/Spawner/Spawner.h"

#include "Gameplay/Wall/BoundingWall.h"
#include <Core/Application.h>
#include "Core/LoggingSystem/LoggingSystem.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<SpawnerManager>("SpawnerManager")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Spawners", &SpawnerManager::m_vSpawnerEntities)(RTTR_PUBLIC, RTTR_TOOLTIP("Spawners connected to this manager"))
		.property("Bounding Walls", &SpawnerManager::m_BoundWalls)(RTTR_PUBLIC, RTTR_TOOLTIP("Walls connected to this manager"))
		.property("Trigger Box Size", &SpawnerManager::GetBoxSize, &SpawnerManager::SetBoxSize)(RTTR_PUBLIC, RTTR_TOOLTIP("Trigger box size"));
}

void SpawnerManager::SetBoxSize(Vector3 v3BoxSize)
{
	if (v3BoxSize == v3InitializeBoxSize)
		return;

	v3InitializeBoxSize = v3BoxSize;

	if (auto pCollider = GetComponentAll<BoxCollider>())
		pCollider->SetBoxSize(v3InitializeBoxSize);
}


void SpawnerManager::Awake()
{
	// Collider
	m_pCollider = AddComponent<BoxCollider>();
	if (!m_pCollider)
		m_pCollider = GetComponent<BoxCollider>();
	m_pCollider->SetBoxSize(v3InitializeBoxSize);
	m_pCollider->SetTrigger(true);
	m_pCollider->SetIgnoreVoxels(true);

	const std::string sName = "SpawnerManager" + std::to_string(GetId());
	SetName(sName);

	SetPersistent(true);
}

void SpawnerManager::Start()
{
	Entity::Start();

	if (m_pCollider)
		m_pCollider->SetBoxSize(v3InitializeBoxSize);

	if(!m_vSpawnerEntities.empty())
	{
		for(uint32_t i = 0; i < m_vSpawnerEntities.size(); ++i)
		{
			if (!m_vSpawnerEntities[i])
			{
				m_vSpawnerEntities.erase(m_vSpawnerEntities.begin() + i);
				GetWorld()->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "Spawner Manager", "Could not find spawner entity, it could possibly be located in a different chunk as the manager: " + GetName());
				continue;
			}

			Spawner* pSpawner = m_vSpawnerEntities[i];

			pSpawner->Destroyed += Event<Component*>::Subscriber([=](Component* pComponent)
			{
				const auto it = std::find(m_vSpawnerEntities.begin(), m_vSpawnerEntities.end(), pComponent);
				if (it != m_vSpawnerEntities.end())
					m_vSpawnerEntities.erase(it);
			}, this);

			pSpawner->OnSpawnerDisabled += Event<Spawner*>::Subscriber([=](Spawner* pComponent)
			{
				// Unsubscribe
				pComponent->Destroyed -= this;

				const auto it = std::find(m_vSpawnerEntities.begin(), m_vSpawnerEntities.end(), pComponent);
				if (it != m_vSpawnerEntities.end())
					m_vSpawnerEntities.erase(it);
			}, this);
		}
	}

	if (!m_BoundWalls.empty())
	{
		// Set all walls to false
		for (auto pWall : m_BoundWalls)
		{
			if (pWall)
				pWall->SetEnabled(false);
		}
	}
}

void SpawnerManager::FixedTick(const ::GameTimer& gameTimer)
{
	Entity::FixedTick(gameTimer);

	// if we defeated all the enemies and destroyed all the spawners
	if(m_vSpawnerEntities.empty())
	{
		if (!m_BoundWalls.empty())
		{
			// Set all walls to false
			for (auto pWall : m_BoundWalls)
			{
				if (pWall)
					pWall->SetEnabled(false);
			}
		}


		Destroy();
	}
}

void SpawnerManager::OnCollisionEnter(Collider* pCollider, const Manifold&)
{
	if (pCollider->GetOwner()->HasTag("Player"))
	{
		// If we don;t have the first player grab it
		if (!m_pPlayer1)
		{
			m_pPlayer1 = pCollider->GetOwner();
		}
		else if(m_pPlayer1 && !m_pPlayer2)
		{
			m_pPlayer2 = pCollider->GetOwner();
		}

		// if we have either first or second player
		if ((m_pPlayer1 || m_pPlayer2) && !m_BoundWalls.empty())
		{
			for (auto pWall : m_BoundWalls)
			{
				if (pWall && pWall->bEndWall)
					pWall->SetEnabled(true);
			}
		}
	}
}

void SpawnerManager::OnCollisionStay(Collider*, const Manifold&)
{
	if(m_pPlayer1 && m_pPlayer2)
	{
		// Start the spawners
		if(!m_vSpawnerEntities.empty())
		{
			for(auto pSpawner : m_vSpawnerEntities)
			{
				if (pSpawner->HasStarted())
					continue;

				pSpawner->StartWave();
			}
		}

		if(!m_BoundWalls.empty())
		{
			for (auto pWall : m_BoundWalls)
			{
				if (pWall && !pWall->IsEnabled())
				{
					pWall->SetEnabled(true);
				}
			}
		}
	}
}

void SpawnerManager::OnCollisionExit(Collider* pCollider, const Manifold&)
{
	// If one of the players is walking out of the trigger box.
	if (m_pPlayer1 == pCollider->GetOwner())
	{
		m_pPlayer1 = nullptr;
	}
	
	if (m_pPlayer2 == pCollider->GetOwner())
	{
		m_pPlayer2 = nullptr;
	}
}
