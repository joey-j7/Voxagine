#include "SpawnerEntity.h"
#include "Spawner.h"

#include <External/rttr/registration>
#include <Core/MetaData/PropertyTypeMetaData.h>
#include <Core/ECS/World.h>
#include <Core/ECS/Components/PhysicsBody.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Components/AudioSource.h>

#include "Core/ECS/Systems/Physics/PhysicsSystem.h"

#include "Gameplay/Wall/BoundingWall.h"

#include "Prefabs/PortalPrefab.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<SpawnerEntity>("SpawnerEntity")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Size", &SpawnerEntity::GetSize, &SpawnerEntity::SetSize)(RTTR_PUBLIC)
		.property("Is Manager", &SpawnerEntity::IsManager, &SpawnerEntity::SetManager)(RTTR_PUBLIC)
		.property("Manager (For non Manager Entities)", &SpawnerEntity::m_pSpawnerMaster)(RTTR_PUBLIC)
		.property("Bounding Walls", &SpawnerEntity::m_BoundWalls)(RTTR_PUBLIC)
		.property("Enable Trigger Music", &SpawnerEntity::m_bTriggerMusic) (RTTR_PUBLIC);
}

#pragma region SpawnerEntity

void SpawnerEntity::SetSize(Vector3 v3Size)
{
	if (v3InitializeBoxSize == v3Size)
		return;

	v3InitializeBoxSize = v3Size;

	if(m_pCollider)
		m_pCollider->SetBoxSize(v3InitializeBoxSize);
}

SpawnerEntity::SpawnerEntity(World* world) : Entity(world) {}

SpawnerEntity::~SpawnerEntity()
{
	if (m_bIsManager)
	{
		SpawningWavesFinished -= this;
	}
		
	// WaveEnded -= this;

	m_pSpawners.clear();
	m_pFinishedWaves.clear();
}

void SpawnerEntity::SetManager(bool bIsManager)
{
	if (m_bIsManager == bIsManager)
		return;

	if(bIsManager)
	{
		AddTag("SpawnerManager");
	} 
	else
	{
		RemoveTag("SpawnerManager");
	}

	m_bIsManager = bIsManager;
}

void SpawnerEntity::AddSpawnerEntity(SpawnerEntity* pEntity)
{
	m_pSpawners.push_back(pEntity);
}

void SpawnerEntity::Awake()
{
	Entity::Awake();
	SetName("SpawnerEntity");
	AddTag("SpawnerEntity");

	/**
	 * @brief - this is the initial state of the spawner entity
	 * - Init physics body --> set gravity off.
	 *
	 * - Add a collider in order to spawn entities when getting hit
	 * --> Set initialize size
	 * --> Set trigger on
	 * --> Ignore voxels on
	 *
	 * - Add the spawner component
	 *
	 */
	auto physicsBody = AddComponent<PhysicsBody>();
	if (!physicsBody)
		physicsBody = GetComponent<PhysicsBody>();
	physicsBody->SetGravity(false);

	// Collider
	m_pCollider = AddComponent<BoxCollider>();
	if (!m_pCollider)
		m_pCollider = GetComponent<BoxCollider>();
	m_pCollider->SetBoxSize(v3InitializeBoxSize);
	m_pCollider->SetTrigger(true);
	m_pCollider->SetIgnoreVoxels(true);

	// Spawner
	m_pSpawnerComponent = AddComponent<Spawner>();
	if (!m_pSpawnerComponent)
		m_pSpawnerComponent = GetComponent<Spawner>();
	// Audio
	m_pAudio = AddComponent<AudioSource>();
	if (!m_pAudio)
		m_pAudio = GetComponent<AudioSource>();

	// Set the initialize values. can be changed later.
	m_pAudio->SetFilePath("Content/Music/Battle_Normal.ogg");
	m_pAudio->SetLooping(true);
	m_pAudio->SetLoopCount(-1);
	m_pAudio->SetLoopPoints(157571, 2671259);

	/* Setup the portal */
	for (Entity* pChild : GetChildren())
	{
		if (pChild->GetName() == "Portal")
		{
			pPortalEntity = dynamic_cast<PortalPrefab*>(pChild);
			break;
		}
	}

	if (!pPortalEntity)
	{
		pPortalEntity = GetWorld()->SpawnEntity<PortalPrefab>(Vector3(), Quaternion(), Vector3());
		pPortalEntity->SetParent(this);
	}
}

void SpawnerEntity::Start()
{
	Entity::Start();
	if (m_pCollider)
		m_pCollider->SetBoxSize(v3InitializeBoxSize);

	// if(m_bIsManager && !m_pSpawnerMaster)
	// {
	// 	// if we dont have the manager we need to find it.
	// 	auto entities = GetWorld()->FindEntitiesWithTag("SpawnerManager");
	// 	if(entities.empty())
	// 	{
	// 		for(auto pEntity : entities)
	// 		{
	// 			if(const auto pSpawner = dynamic_cast<SpawnerEntity*>(pEntity))
	// 			{
	// 				if (pSpawner->m_bIsManager)
	// 				{
	// 					m_pSpawnerMaster = pSpawner;
	// 					break;
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	// so now if we have the manager add our self
	if(!m_bIsManager && m_pSpawnerMaster && m_pSpawnerMaster != this)
	{
		m_pSpawnerMaster->AddSpawnerEntity(this);
	}


	if (m_bIsManager && !m_BoundWalls.empty())
	{
		// Set all walls to false
		for (auto pWall : m_BoundWalls)
		{
			if(pWall)
				pWall->SetEnabled(false);
		}
	}

	if (m_bIsManager) // if we are the manager we have to registrate the events.
	{
		SpawningWavesFinished += Event<>::Subscriber(std::bind(&SpawnerEntity::OnSpawningWavesFinished, this), this);
	}

	// for normal spawners to tell the master that we are done with the wave
	WaveEnded += Event<SpawnerEntity*>::Subscriber(std::bind(&SpawnerEntity::OnWaveEnded, this, std::placeholders::_1), this);


	if (m_pPlayer1)
		m_pPlayer1->Destroyed += Event<Entity*>::Subscriber([=](Entity* pEntity)
	{
		m_pPlayer1->Destroyed -= this;
		m_pPlayer1 = nullptr;
	}, this);

	if (m_pPlayer2)
		m_pPlayer2->Destroyed += Event<Entity*>::Subscriber([=](Entity* pEntity)
	{
		m_pPlayer2->Destroyed -= this;
		m_pPlayer2 = nullptr;
	}, this);
}

void SpawnerEntity::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);

	DebugRenderer* pDebugRenderer = GetWorld()->GetDebugRenderer();
	if (pDebugRenderer)
		pDebugRenderer->AddCenteredBox(GetTransform()->GetPosition(), v3InitializeBoxSize, VColor(VColors::Green));


	if (m_pSpawnerComponent && !m_pSpawnerComponent->IsEnabled())
	{
		// if we are not the manager we need to notify that we are getting destroyed.
		if(!m_bIsManager)
		{
			SpawningWavesFinished();

			Destroy();
		}


		if (m_bIsManager && !m_BoundWalls.empty())
		{
			if (m_pSpawners.empty() || uiCurrentCounter == m_pSpawners.size() + 1 /* because myself the master */)
			{
				for (auto Wall : m_BoundWalls)
				{
					Wall->SetEnabled(false);

					// Destroy the walls
					Wall->Destroy();
				}

				Destroy();
			}

			// We might be the last one that has survived. Time the clear everything out.
			OnSpawningWavesFinished();
		}
	}

	// Check if we are done with all the wave rounds if we have registrated spanwers
	if(m_pSpawners.empty() || !m_pSpawners.empty() && uiWaveCounter >= m_pSpawners.size() + 1 /* because myself the master */) // if we dont have any connected spawners then we will just continue
	{
		if (m_bIsManager)
		{
			if (!m_pSpawners.empty())
			{
				for (auto pSpawner : m_pSpawners)
				{
					pSpawner->m_bShouldWait = false;
				}
			}

			// Including myself should play
			m_bShouldWait = false;
			uiWaveCounter = 0;
			m_pFinishedWaves.clear();
		}
		else 
		{
			// if we don't have a master but we have more waves just continue.
			if (!m_pSpawnerMaster)
			{
				m_bShouldWait = false;
				uiWaveCounter = 0;
			}
		}
	}
}

void SpawnerEntity::PostTick(float fDeltaTime)
{
	Entity::PostTick(fDeltaTime);
}

void SpawnerEntity::OnDrawGizmos(float fDeltaTime)
{
	if(m_pSpawnerComponent)
	{
		m_pSpawnerComponent->OnDrawGizmos(fDeltaTime);
	}
}

void SpawnerEntity::OnCollisionEnter(Collider * collider, const Manifold & manifold)
{
	if (m_pSpawnerComponent)
	{
		auto& tags = collider->GetOwner()->GetTags();
		if (std::find(tags.begin(), tags.end(), "Player") != tags.end())
		{
			if (m_pSpawnerComponent && m_pSpawnerComponent->HasStarted())
				return;

			// Set music
			// if (m_pAudio && !m_pAudio->IsPlaying() && m_bTriggerMusic)
				// m_pAudio->SetAsBGM();

			//  if we don;t have any players
			if(!m_pPlayer1 && !m_pPlayer2)
			{
				m_pPlayer1 = collider->GetOwner();
			}
			else if(m_pPlayer1 && !m_pPlayer2)
			{
				m_pPlayer2 = collider->GetOwner();
			}

			if (m_bIsManager)
			{
				// if we have either first or second player
				if ((m_pPlayer1 || m_pPlayer2) && !m_BoundWalls.empty())
				{
					for (auto pWall : m_BoundWalls)
					{
						if (pWall && pWall->bEndWall)
							pWall->SetEnabled(true);
					}

				}

				// if we are the manager enable walls
				if (m_pPlayer1 && m_pPlayer2 && !m_BoundWalls.empty())
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

			// If we have both players start the spawner
			if (m_pPlayer1 && m_pPlayer2)
			{
				m_pSpawnerComponent->StartWave();
			}
		}
	}
}

void SpawnerEntity::OnWaveEnded(SpawnerEntity* pSpawner)
{
	if(m_bIsManager)
	{
		uiWaveCounter++;
	}
	else  // if we have an master and we are not master
	{
		const auto it = std::find(m_pFinishedWaves.begin(), m_pFinishedWaves.end(), pSpawner);
		if (m_pSpawnerMaster && it == m_pFinishedWaves.end()) // check if we have an manager
		{
			m_pSpawnerMaster->uiWaveCounter++;
			m_pFinishedWaves.push_back(pSpawner);
		}
		else
			// if we don't have a master then we can just continue
			m_bShouldWait = false;
	}
}

void SpawnerEntity::OnSpawningWavesFinished()
{
	uiCurrentCounter++;
}

#pragma endregion 