#include "Spawner.h"

#include <Core/ECS/World.h>
#include <Core/ECS/Entity.h>
#include <Core/ECS/Components/Transform.h>
#include <Core/ECS/Components/BoxCollider.h>
#include <Core/ECS/Components/VoxRenderer.h>
#include <Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "Humanoids/ParticleCorpse.h"
#include "General/FlashBehavior.h"
#include "Humanoids/Enemies/RandomMonster.h"
#include "Humanoids/Enemies/UmbrellaMonster.h"
#include "Humanoids/Enemies/LongNeckMonster.h"
#include "Humanoids/Enemies/SpiderMonster.h"

RTTR_REGISTRATION
{
	rttr::registration::enumeration<SpawnerType>("SpawnerType")
	(
		rttr::value("Normal",	SpawnerType::ST_Normal),
		rttr::value("Gauntlet",	SpawnerType::ST_Gauntlet)
	);

	rttr::registration::class_<Spawner>("Spawner")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Spawner Type", &Spawner::m_eSpawnerType)(RTTR_PUBLIC, RTTR_TOOLTIP("Define the type of the Spawner"))
		.property("Monster Type", &Spawner::m_MonsterClass)(RTTR_PUBLIC, RTTR_TOOLTIP("Monster type that will spawn"))
		.property("Amount of Enemies", &Spawner::m_iAmountEnemies)(RTTR_PUBLIC, RTTR_TOOLTIP("Amount of enemies each wave"))
		.property("Amount Enemies Alive", &Spawner::m_iAmountEnemiesAlive)(RTTR_PUBLIC, RTTR_TOOLTIP("Amount of enemies before spawning new ones"))
		.property("Amount of Waves", &Spawner::m_iAmountWaves)(RTTR_PUBLIC, RTTR_TOOLTIP("Amount of waves"))

		.property("Auto Start Waves", &Spawner::m_bAutoStart)(RTTR_PUBLIC, RTTR_TOOLTIP("Allows independent use without a spawner manager"))

		.property("Respawnable", &Spawner::m_bRespawnable)(RTTR_PUBLIC, RTTR_TOOLTIP("Make the spawner respawn after a specified time, don't use for progressing levels"))
		.property("Respawn Time Min", &Spawner::GetRespawnTimeMin, &Spawner::SetRespawnTimeMin)(RTTR_PUBLIC, RTTR_TOOLTIP("Minimum respawn time range"))
		.property("Respawn Time Max", &Spawner::GetRespawnTimeMax, &Spawner::SetRespawnTimeMax)(RTTR_PUBLIC, RTTR_TOOLTIP("Maximum respawn time range"))

		.property("Instant Spawn", &Spawner::m_bBurst)(RTTR_PUBLIC, RTTR_TOOLTIP("Spawn x amount immediately"))
		.property("Start Spawn timer", &Spawner::m_fSpawnTimer)(RTTR_PUBLIC, RTTR_TOOLTIP("Start timer of the spawning"))
		.property("Cooldown timer", &Spawner::m_fResetSpawnTimer)(RTTR_PUBLIC, RTTR_TOOLTIP("How long should we wait for the next wave"))
		.property("Max Health", &Spawner::m_fMaxHealth)(RTTR_PUBLIC, RTTR_TOOLTIP("Max health of the gauntlet"))
		.property("Immortal", &Spawner::m_bImmortal)(RTTR_PUBLIC, RTTR_TOOLTIP("Toggle the gauntlet to (im)mortal"))
		.property("Spawn Radius", &Spawner::fRadius)(RTTR_PUBLIC, RTTR_TOOLTIP("Radius of whhere the enemies will spawn"))
		.property("AI Group", &Spawner::m_Group)(RTTR_PUBLIC, RTTR_TOOLTIP("Which ai group the monster will be part of."))
		.property("Not Visible", &Spawner::GetInvisible, &Spawner::SetInvisible)(RTTR_PUBLIC, RTTR_TOOLTIP("If the Spawner should be visible or not"))
		.property("Maximal Force", &Spawner::v3MaxForce)(RTTR_PUBLIC, RTTR_TOOLTIP("Maximal force when exploding"));
}

Spawner::~Spawner()
{
	if(!m_vSpawnedEnemies.empty())
	{
		for (auto pMonsterEntity : m_vSpawnedEnemies)
		{
			pMonsterEntity->Destroyed -= this;
		}
	}
}

// We need to destroy our self when we loaded the amount waves
bool Spawner::ShouldSpawning() const
{
	return m_vSpawnedEnemies.size() <= m_iAmountEnemiesAlive && (m_iCurrentCounter <= m_iAmountWaves || m_eSpawnerType == SpawnerType::ST_Gauntlet) && !m_bIsInvisible;
}

void Spawner::StartWave()
{
	if (m_bIsStarted)
		return;

	if(m_bIsInvisible)
	{
		m_bIsInvisible = false;

		if (m_pVoxRenderer)
			m_pVoxRenderer->SetEnabled(true);
	}

	m_fScaleTimer = 0.f;
	GetTransform()->SetScale(Vector3(0.f));

	m_bIsStarted = true;
}

void Spawner::Start()
{
	BehaviorScript::Start();

	pPortalEntity = GetOwner();
	m_pFlashBehavior = pPortalEntity->GetComponent<FlashBehavior>();

	if (auto pCollider = GetOwner()->GetComponentAll<BoxCollider>()) pCollider->AutoFit(true);

	m_fHealth = m_fMaxHealth;

	// if we are not part of the group because we are disabled then notify the manager
	if (!IsEnabled())
		OnSpawnerDisabled(this);

	m_pVoxRenderer = GetOwner()->GetComponentAll<VoxRenderer>();

	if (m_pVoxRenderer && m_bIsInvisible)
		m_pVoxRenderer->SetEnabled(false);

	m_InitialScale = GetTransform()->GetScale();

	if (m_bAutoStart && !m_bIsInvisible)
	{
		m_iCurrentCounter = 0;
		m_bIsDestroyed = false;

		m_fHealth = m_fMaxHealth;
		m_fSpawnTimer = m_fResetSpawnTimer;

		if (auto pCollider = GetOwner()->GetComponentAll<BoxCollider>())
		{
			pCollider->SetTrigger(false);
			pCollider->SetEnabled(true);
		}

		m_bIsStarted = false;
		m_bIsInvisible = true;

		StartWave();
	}
}

void Spawner::FixedTick(const GameTimer& gameTimer)
{
	BehaviorScript::FixedTick(gameTimer);

	if (m_fScaleTimer < 1.f && !m_bIsInvisible)
	{
		m_fScaleTimer += gameTimer.GetElapsedSeconds();
		GetTransform()->SetScale(m_InitialScale * std::min(1.f, m_fScaleTimer * 2.f));
	}

	if (m_bIsDestroyed)
	{
		if (m_vSpawnedEnemies.empty())
		{
			m_bIsAlive = true;
			m_fRespawnTimer += gameTimer.GetElapsedSeconds();

			if (m_fRespawnTimer >= m_fRespawnTime)
			{
				m_iCurrentCounter = 0;
				m_bIsDestroyed = false;

				m_fHealth = m_fMaxHealth;
				m_fSpawnTimer = m_fResetSpawnTimer;

				if (auto pCollider = GetOwner()->GetComponentAll<BoxCollider>())
				{
					pCollider->SetTrigger(false);
					pCollider->SetEnabled(true);
				}

				m_bIsStarted = false;
				m_bIsInvisible = true;

				StartWave();
			}
		}

		return;
	}

	// if we are not starting and not alive destroy our self
	if (!m_bIsAlive && m_vSpawnedEnemies.empty())
	{
		Dead();
	} 

	// if all the enemies are dead spawn new ones.
	if(m_bIsStarted)
	{
		if (ShouldSpawning())
		{
			m_fSpawnTimer -= gameTimer.GetElapsedSeconds();
			if (m_fSpawnTimer <= 0.0f)
			{
				// Spawn the enemies
				SpawnEnemies();
				m_fSpawnTimer = m_fResetSpawnTimer;
			}
		} 
		
		// see if we should keep playing because of current counter and the amount 
		// of enemies in the game from this spawner
		// And if we are a normal spawner
		if (m_iCurrentCounter >= m_iAmountWaves && m_eSpawnerType == SpawnerType::ST_Normal)
		{
			m_bIsAlive = m_bIsStarted = false;
		}
	}
}

void Spawner::Damage(float fDamage, Vector3 v3ImpactNormal)
{
	// If we are dead we need to disable our component to render
	// because then we can keep the check for the entities before
	// destroying our self.
	// But cant spawn any more entities
	if ((m_eSpawnerType != SpawnerType::ST_Gauntlet || !m_bIsAlive))
		return;

	// if we are not visible return as well.
	if (m_bIsInvisible)
		return;

	if (m_pFlashBehavior)
		m_pFlashBehavior->StartFlashing();

	m_v3ImpactNormal = v3ImpactNormal;

	if (!m_bImmortal) 
	{ // only when we are not immortal	
		m_fHealth -= fDamage;
		if (m_fHealth < 0.0f)
		{
			m_fHealth = 0.0f;
		}
	}

	if (m_fHealth <= 0.0f)
	{
		m_bIsAlive = false;

		// Set the current counter immediately to the max wave
		// counter in order to don't spawn anymore
		// monsters.
		m_iCurrentCounter = m_iAmountWaves;

		// Turn of our renderer and box collider to take no more hits
		Shatter();

		if (m_pVoxRenderer) m_pVoxRenderer->SetEnabled(false);
		if (auto pCollider = GetOwner()->GetComponentAll<BoxCollider>())
		{
			pCollider->SetTrigger(true);
			pCollider->SetEnabled(false);
		}

		m_bIsInvisible = true;

		if (m_bRespawnable)
		{
			m_bIsDestroyed = true;
			m_fRespawnTimer = 0.f;

			m_fRespawnTime = glm::linearRand(m_fRespawnTimeMin, m_fRespawnTimeMax);

			SetInvisible(true);
			m_bIsStarted = false;
		}
	}
}

void Spawner::SetInvisible(bool bVisible)
{
	if (m_bIsInvisible == bVisible)
		return;

	m_bIsInvisible = bVisible;

	if (auto pVoxRender = GetOwner()->GetComponentAll<VoxRenderer>())
		pVoxRender->SetEnabled(!m_bIsInvisible);
}

void Spawner::Dead()
{
	// the gauntlet already shattered to pieces.
	if (m_eSpawnerType != SpawnerType::ST_Gauntlet)
	{
		Shatter();
	}

	if (!m_bRespawnable)
	{
		GetOwner()->Destroy();
	}
}

void Spawner::Shatter() const
{
	const auto OwnerPos = (pPortalEntity) ? pPortalEntity->GetTransform()->GetPosition() : GetOwner()->GetTransform()->GetPosition();
	const auto OwnerRot = (pPortalEntity) ? pPortalEntity->GetTransform()->GetRotation() : GetOwner()->GetTransform()->GetRotation();
	const auto OwnerScale = (pPortalEntity) ? pPortalEntity->GetTransform()->GetScale() : GetOwner()->GetTransform()->GetScale();

	auto particleEntity = GetWorld()->SpawnEntity<ParticleCorpse>(OwnerPos, OwnerRot, OwnerScale);
	particleEntity->m_voxFile = m_pVoxRenderer->GetModelFilePath();
	particleEntity->m_MinForce = v3MaxForce;
	particleEntity->m_MaxForce = -v3MaxForce;
	particleEntity->m_fArcAngle = 180.f;
}

Monster* Spawner::SpawnEnemyAtRandomLocation() const
{
	float x = (pPortalEntity) ? pPortalEntity->GetTransform()->GetPosition().x : GetOwner()->GetTransform()->GetPosition().x;
	float z = (pPortalEntity) ? pPortalEntity->GetTransform()->GetPosition().z : GetOwner()->GetTransform()->GetPosition().z;

	x += fRadius * cosf(static_cast<float>(rand() % Utils::FULL_CIRCLE_RADIANS));
	z += fRadius * sinf(static_cast<float>(rand() % Utils::FULL_CIRCLE_RADIANS));

	const Vector3 location = { x, (pPortalEntity) ? pPortalEntity->GetTransform()->GetPosition().y : 20.0f, z };

	/*const float fRadius = 10.0f;

	std::vector<BoxCollider*> pColliders = {};
	if (GetWorld()->GetPhysics()->OverlapSphere(pColliders, location, fRadius, CL_OBSTACLES))
	{
		if (!pColliders.empty())
		{
			Vector2 v2Closest = Vector2(), v2NewLocation;
			for (auto& pCollider : pColliders)
			{
				// get the distance from the collider to the portal
				Transform* pTransform = (pPortalEntity) ? pPortalEntity->GetTransform() : GetOwner()->GetTransform();

				// use the dot product to see the portal is on the bottom or top.
				Vector3 a = pCollider->GetTransform()->GetForward(); // Box's forward vector
				Vector3 b = glm::normalize(pTransform->GetPosition() - pCollider->GetTransform()->GetPosition()); // Vector from Box to Portal

				const float fDot = glm::dot(a, b);

				Vector3 dist = pCollider->GetTransform()->GetPosition() + pTransform->GetPosition();

				if(fDot < 0.0f)
				{ // We are behind the box
					dist -= pCollider->GetBoxSize() * 0.5f;
				} 
				else if(fDot > 0.0f)
				{ // we are 
					dist += pCollider->GetBoxSize() * 0.5f;
				}
				else
				{ // we are either on the left or right side
					if(pTransform->GetPosition().x < pCollider->GetTransform()->GetPosition().x) // right side


					dist += pCollider->GetBoxSize() * 0.5f;
				}

				// divide it by 2
				dist *= 0.5f;

				// set that distance
				x = dist.x;
				z = dist.z;

				// add randomness
				x += fRadius * cosf(static_cast<float>(rand() % Utils::FULL_CIRCLE_RADIANS));
				z += fRadius * sinf(static_cast<float>(rand() % Utils::FULL_CIRCLE_RADIANS));

				v2NewLocation.x = x;
				v2NewLocation.y = z;

				if (v2NewLocation.x < v2Closest.x && v2NewLocation.y < v2Closest.y || Vector2() == v2Closest)
					v2Closest = v2NewLocation;
			}

			location.x = v2Closest.x;
			location.z = v2Closest.y;
		}
	}*/

	rttr::type classType = m_MonsterClass.get_derived_type();

	if (classType == rttr::type::get<RandomMonster>())
	{
		uint32_t uiRandom = static_cast<uint32_t>(glm::floor(glm::linearRand(0.f, 3.f - FLT_MIN)));

		if (uiRandom == 1)
			classType = rttr::type::get<LongNeckMonster>();
		else if (uiRandom == 2)
			classType = rttr::type::get<SpiderMonster>();
		else
			classType = rttr::type::get<UmbrellaMonster>();
	}

	return static_cast<Monster*>(GetWorld()->SpawnEntity(classType, location, Vector3(), Vector3(1.0f)));
}

void Spawner::SpawnEnemies()
{
	if (m_bIsDestroyed || m_bIsInvisible)
		return;

	for (size_t i = 0; i < m_iAmountEnemies; ++i)
	{
		Monster* pMonsterEntity = SpawnEnemyAtRandomLocation();

		if (m_Group)
		{
			auto pPathfinder = pMonsterEntity->GetComponentAll<pathfinding::Pathfinder>();
			if (!pPathfinder)
				pPathfinder = pMonsterEntity->AddComponent<pathfinding::Pathfinder>();

			pPathfinder->m_group = m_Group;
			m_Group->addAgent(*pPathfinder);
		}

		pMonsterEntity->Destroyed += Event<Entity*>::Subscriber([=](Entity* pEntity)
		{
			const auto it = std::find(m_vSpawnedEnemies.begin(), m_vSpawnedEnemies.end(), pEntity);
			if (it != m_vSpawnedEnemies.end())
				m_vSpawnedEnemies.erase(it);
		}, this);


		m_vSpawnedEnemies.push_back(pMonsterEntity);

	}

	// Increment the times we spawned.
	m_iCurrentCounter++;
}
