#include "pch.h"
#include "Core/ECS/World.h"

#include "Core/Application.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Core/ECS/Systems/Chunk/ChunkSystem.h"
#include "Core/ECS/Systems/ScriptSystem.h"
#include "Core/ECS/Entities/Camera.h"
#include "Core/ECS/Components/VoxRenderer.h"

#include <typeinfo>
#include <typeindex>
#include "Core/ECS/Components/Transform.h"
#include "Components/VoxAnimator.h"
#include "Core/LoggingSystem/LoggingSystem.h"
#include "Systems/AudioSystem.h"
#include "Core/Threading/JobManager.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingChunkGrid.h"
#include "Core/ECS/Systems/Pathfinding/Grid/PathfindingObstacle.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/PathfinderGroup.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/ContinuumCrowdsGroup.h"
#include "Core/ECS/Systems/Pathfinding/Navigation/Pathfinder.h"

#include "Entities/UI/Canvas.h"
#include "External/optick/optick.h"

World::World(Application* pApp)
{
	m_WorldName = "";
	m_pApplication = pApp;
	m_pRenderSystem = nullptr;
	m_pPhysicsSystem = nullptr;
	m_pChunkSystem = nullptr;
	m_pAudioSystem = nullptr;
	m_pCameraEntity = nullptr;
	m_bPreLoaded = false;

	Canvas p(this);
}

World::~World()
{
}

void World::Initialize()
{
	OPTICK_EVENT();
	m_JobQueueHandle = m_pApplication->GetJobManager().CreateJobQueue();

	if (!m_bPreLoaded)
	{
		ScriptSystem* pScriptSystem = new ScriptSystem(this);
		PhysicsSystem* pPhysicsSystem = new PhysicsSystem(this, Vector3(256, 128, 256), 1);
		SetPhysicsSystem(pPhysicsSystem);

		AudioSystem* pAudioSystem = new AudioSystem(this);
		SetAudioSystem(pAudioSystem);

		ChunkSystem* pChunkSystem = new ChunkSystem(this);
		SetChunkSystem(pChunkSystem);

		m_Systems.push_back(pScriptSystem);
		m_Systems.push_back(pPhysicsSystem);
		m_Systems.push_back(pAudioSystem);
		m_Systems.push_back(pChunkSystem);

		SetRenderSystem(new RenderSystem(this));
	}

	if (GetMainCamera() == nullptr)
	{
		Camera* MainCamera = new Camera(this);
		MainCamera->SetName("Main Camera");
		AddEntity(MainCamera);

		SetMainCamera(MainCamera);
	}

	for (ComponentSystem* pSystem : m_Systems)
		pSystem->Start();

	m_pRenderSystem->Start();

	if (!m_GroundTexturePath.empty())
		SetGroundTexturePath(m_GroundTexturePath);

	Entity entity(nullptr); // Do not remove! Prevents compiler optimization.
	volatile pathfinding::ChunkGrid chunkGrid(this); // Do not remove! Prevents compiler optimization.
	volatile pathfinding::ContinuumCrowdsGroup contiuumCrowdsGroup(this); // Do not remove! Prevents compiler optimization.
	volatile pathfinding::PathfinderGroup* pathfinderGroup = &contiuumCrowdsGroup; // Do not remove! Prevents compiler optimization.
	volatile pathfinding::Pathfinder pathfinder(&entity); // Do not remove! Prevents compiler optimization.
	volatile pathfinding::PathfindingObstacle obstacle(&entity); // Do not remove! Prevents compiler optimization.
}

bool World::PreLoad(const std::string& filePath)
{
	OPTICK_EVENT();
	if (m_pApplication->GetSerializer().DeserializeWorldFromFile(*this, filePath))
	{
		PreLoad();
		m_WorldName = filePath;
		return true;
	}
	return false;
}

void World::PreLoad()
{
	OPTICK_EVENT();
	m_bPreLoaded = true;
	m_Systems.push_back(new ScriptSystem(this));

	if (m_pAudioSystem)
		m_Systems.push_back(m_pAudioSystem);

	if (m_pPhysicsSystem)
		m_Systems.push_back(m_pPhysicsSystem);

	if (m_pChunkSystem)
		m_Systems.push_back(m_pChunkSystem);

	SetRenderSystem(new RenderSystem(this));
}

void World::Unload()
{
	OPTICK_EVENT();
	m_pApplication->GetJobManager().DiscardJobQueue(m_JobQueueHandle);

	for (Entity* pEntity : m_Entities)
	{
		pEntity->Destroy();
	}

	for (Entity* pAddEntity : m_AddedEntities)
	{
		pAddEntity->Destroy();
	}

	for (Entity* pRemoveEntity : m_RemovedEntities)
	{
		DeleteEntity(pRemoveEntity);
	}

	for (ComponentSystem* system : m_Systems)
	{
		delete system;
		system = nullptr;
	}

	delete m_pRenderSystem;
	m_pRenderSystem = nullptr;

	m_RemovedEntities.clear();
	m_Systems.clear();
}

void World::Pause()
{
	OPTICK_EVENT();
	Paused(this);
	m_pApplication->GetJobManager().ShelveJobQueue(m_JobQueueHandle);
}

void World::Resume()
{
	OPTICK_EVENT();
	m_pApplication->GetJobManager().UnShelveJobQueue(m_JobQueueHandle);

	Resumed(this);
	
	GetRenderSystem()->Reveal();

	if (m_pCameraEntity)
		m_pCameraEntity->ForceUpdate();
}

void World::PreTick()
{
	OPTICK_CATEGORY("Gameplay-Pretick", Optick::Category::GameLogic);
	OPTICK_EVENT();

	/* Delete entities queued for deletion */
	for (Entity* pEntity : m_RemovedEntities)
		DeleteEntity(pEntity);
	m_RemovedEntities.clear();

	/* Process new entities to world */
	for (Entity* pEntity : m_AddedEntities)
	{
		if (!pEntity->IsDestroyed())
		{
			m_Entities.push_back(pEntity);
			EntityAdded(pEntity);
		}
	}
	m_AddedEntities.clear();

	/* Update already existing entities if any new components have been added on runtime */
	for (Entity* pEntity : m_Entities)
		pEntity->PreTick();

	// Resolve the links in the world
	GetApplication()->GetSerializer().ResolveWorldLinks(*this);
}

void World::Tick(float fDeltaTime)
{
	/* Tick the entities and systems */
	for (Entity* pEntity : m_Entities)
		pEntity->Tick(fDeltaTime);

	for (ComponentSystem* pSystem : m_Systems)
		pSystem->Tick(fDeltaTime);

	m_pRenderSystem->Tick(fDeltaTime);
}

void World::FixedTick(const GameTimer& fixedTimer)
{
	for (Entity* pEntity : m_Entities)
		pEntity->FixedTick(fixedTimer);

	for (ComponentSystem* pSystem : m_Systems)
		pSystem->FixedTick(fixedTimer);

	m_pRenderSystem->FixedTick(fixedTimer);
}

void World::PostFixedTick(const GameTimer& fixedTimer)
{
	for (Entity* pEntity : m_Entities)
		pEntity->PostFixedTick(fixedTimer);

	for (ComponentSystem* pSystem : m_Systems)
		pSystem->PostFixedTick(fixedTimer);

	m_pRenderSystem->PostFixedTick(fixedTimer);
}

void World::PostTick(float fDeltaTime)
{
	for (Entity* pEntity : m_Entities)
		pEntity->PostTick(fDeltaTime);

	for (ComponentSystem* pSystem : m_Systems)
		pSystem->PostTick(fDeltaTime);

	m_pRenderSystem->PostTick(fDeltaTime);
}

void World::OnDrawGizmos(float fDeltaTime)
{
	OPTICK_EVENT();
#if defined(EDITOR) || defined(_DEBUG)
	for (Entity* pEntity : m_Entities)
		pEntity->OnDrawGizmos(fDeltaTime);

	for (ComponentSystem* pSystem : m_Systems)
		pSystem->OnDrawGizmos(fDeltaTime);

	m_pRenderSystem->OnDrawGizmos(fDeltaTime);
#endif
}

void World::Render(const GameTimer& fixedTimer)
{
	OPTICK_CATEGORY("Rendering", Optick::Category::Rendering);
	OPTICK_EVENT();
	m_pRenderSystem->Render(fixedTimer);
}

void World::RegisterComponent(Component* pComponent)
{
	OPTICK_EVENT();
	for (ComponentSystem* pSystem : m_Systems)
	{
		if (pSystem->CanProcessComponent(pComponent))
			pSystem->AddComponent(pComponent);
	}

	if (m_pRenderSystem->CanProcessComponent(pComponent))
		m_pRenderSystem->AddComponent(pComponent);
}

void World::RegisterComponents(const std::vector<Component*>& components)
{
	OPTICK_EVENT();
	for (Component* pComponent : components)
	{
		for (ComponentSystem* pSystem : m_Systems)
		{
			if (pSystem->CanProcessComponent(pComponent))
				pSystem->AddComponent(pComponent);
		}

		if (m_pRenderSystem->CanProcessComponent(pComponent))
			m_pRenderSystem->AddComponent(pComponent);
	}
}

void World::AddEntity(Entity* pEntity)
{
	OPTICK_EVENT();
	m_AddedEntities.push_back(pEntity);
}

void World::AddRootEntity(Entity* pRootEntity)
{
	OPTICK_EVENT();
	m_AddedEntities.push_back(pRootEntity);
	for (Entity* pChild : pRootEntity->GetChildren())
		AddRootEntity(pChild);
}

void World::RemoveEntity(Entity* pEntity)
{
	OPTICK_EVENT();
	pEntity->DeInitialize();
	m_RemovedEntities.push_back(pEntity);
}

void World::SetSystems(std::vector<ComponentSystem*> systems)
{
	m_Systems = systems;
}

Entity* World::SpawnEntity(rttr::type entityType, Vector3 position, Quaternion rotation, Vector3 scale)
{
	OPTICK_EVENT();
	if (!rttr::type::get<Entity>().is_base_of(entityType))
	{
		m_pApplication->GetLoggingSystem().Log(LOGLEVEL_WARNING, "World", "Failed to spawn entity because its not base of Entity, Type: " + std::string(entityType.get_name()));
		return nullptr;
	}

	rttr::variant entityVar = entityType.create({ this });
	Entity* pEntity = entityVar.get_value<Entity*>();
	Transform* transform = pEntity->GetTransform();
	transform->SetPosition(position);
	transform->SetRotation(rotation);
	transform->SetScale(scale);
	AddEntity(pEntity);

	return pEntity;
}

DebugRenderer* World::GetDebugRenderer() const
{
	return &m_pRenderSystem->GetDebugRenderer();
}

Entity* World::FindEntity(std::string name)
{
	OPTICK_EVENT();
	for (Entity* pEntity : m_Entities)
	{
		if (pEntity->GetName() == name)
			return pEntity;
	}
	return nullptr;
}

Entity* World::FindEntity(uint64_t uiId)
{
	OPTICK_EVENT();
	for (Entity* pEntity : m_Entities)
	{
		if (pEntity->GetId() == uiId)
			return pEntity;
	}
	return nullptr;
}

Entity* World::FindEntityAll(uint64_t uiId)
{
	OPTICK_EVENT();
	for (Entity* pEntity : m_Entities)
	{
		if (pEntity->GetId() == uiId)
			return pEntity;
	}

	for (Entity* pEntity : m_AddedEntities)
	{
		if (pEntity->GetId() == uiId)
			return pEntity;
	}

	for (Entity* pEntity : m_RemovedEntities)
	{
		if (pEntity->GetId() == uiId)
			return pEntity;
	}
	return nullptr;
}

std::vector<Entity*> World::FindEntities(std::string name)
{
	OPTICK_EVENT();
	std::vector<Entity*> foundEntities;
	for (Entity* pEntity : m_Entities)
	{
		if (pEntity->GetName() == name)
			foundEntities.push_back(pEntity);
	}
	return foundEntities;
}

std::vector<Entity*> World::FindEntitiesWithTag(std::string tag)
{
	OPTICK_EVENT();
	std::vector<Entity*> foundEntities;
	for (Entity* pEntity : m_Entities)
	{
		if (pEntity->HasTag(tag))
			foundEntities.push_back(pEntity);
	}
	return foundEntities;
}

bool World::FindEntityWithTag(std::string tag)
{
	OPTICK_EVENT();
	for (Entity* pEntity : m_Entities)
	{
		if (pEntity->HasTag(tag))
			return true;
	}
	return false;
}

const std::vector<Entity*>& World::GetEntities()
{
	return m_Entities;
}

const std::vector<Entity*>& World::GetAddedEntities()
{
	return m_AddedEntities;
}

bool World::RayCast(Vector3 start, Vector3 dir, HitResult& hitResult, float fLength, uint32_t uiLayer)
{
	return m_pPhysicsSystem->RayCast(start, dir, hitResult, fLength, uiLayer);
}

const VoxelGrid* World::GetVoxelGrid()
{
	if (m_pPhysicsSystem)
		return m_pPhysicsSystem->GetVoxelGrid();
	return nullptr;
}

void World::ApplySphericalDestruction(const Vector3& position, float fRadius, float fForceMin, float fForceMax, bool bBakeParticles /*= true*/)
{
	m_pPhysicsSystem->ApplySphericalDestruction(position, fRadius, fForceMin, fForceMax, bBakeParticles);
}

float World::GetDeltaSeconds()
{
	return static_cast<float>(m_pApplication->GetTimer().GetElapsedSeconds());
}

float World::GetTotalSeconds()
{
	return static_cast<float>(m_pApplication->GetTimer().GetTotalSeconds());
}

bool World::IsPreLoaded() const
{
	return m_bPreLoaded;
}

Application* World::GetApplication() const
{
	return m_pApplication;
}

void World::SetMainCamera(Camera * pCamera)
{
	if (m_pCameraEntity)
		m_pCameraEntity->SetMainCamera(false);
	
	m_pCameraEntity = pCamera;
	pCamera->SetMainCamera(true);
}

UVector2 World::GetWorldSize() const
{
	return m_pChunkSystem->GetWorldSize();
}

void World::SetGroundTexturePath(const std::string& texturePath)
{
	m_GroundTexturePath = texturePath;

	if (m_pChunkSystem)
		m_pChunkSystem->SetGroundPlane(m_GroundTexturePath);

	if (m_pRenderSystem)
		m_pRenderSystem->SetGroundPlane(m_GroundTexturePath);
}

void World::OpenWorld(const std::string& worldName, bool bReplace /* = true */)
{
	OPTICK_EVENT();
	World* pNewWorld = nullptr;
	World* pPreviousWorld = nullptr;

	size_t worldCount = m_pApplication->GetWorldManager().GetWorldCount();
	if (worldCount > 1)
		pPreviousWorld = m_pApplication->GetWorldManager().GetWorlds()[worldCount - 2];

	// Pop current world if we want to load the previous world without replacing
	if (!bReplace && pPreviousWorld && pPreviousWorld->GetName() == worldName)
	{
		m_pApplication->GetWorldManager().PopWorld();
		return;
	}
	
	// Return the newly preloaded world
	pNewWorld = new World(m_pApplication);
	if (pNewWorld->PreLoad(worldName))
	{
		if (bReplace)
			m_pApplication->GetWorldManager().LoadWorld(pNewWorld);
		else m_pApplication->GetWorldManager().PushWorld(pNewWorld);

		return;
	}

	// Display error because world couldn't be loaded
	m_pApplication->GetLoggingSystem().Log(LOGLEVEL_ERROR, "World", "Couldn't open world with path: " + worldName);
	delete pNewWorld;
}

void World::OpenWorldAsync(const std::string& worldName, bool bReplace /*= true*/)
{
	JobQueue* pJobQueue = GetJobQueue();
	if (!pJobQueue) return;

	pJobQueue->Enqueue<World*>([this, worldName, bReplace]()
	{
		World* pNewWorld = nullptr;
		World* pPreviousWorld = nullptr;

		size_t worldCount = m_pApplication->GetWorldManager().GetWorldCount();
		if (worldCount > 1)
			pPreviousWorld = m_pApplication->GetWorldManager().GetWorlds()[worldCount - 2];

		// Don't reload previous world if we don't want to replace
		if (!bReplace && pPreviousWorld && pPreviousWorld->GetName() == worldName)
		{
			return pPreviousWorld;
		}

		// Return the newly preloaded world
		pNewWorld = new World(m_pApplication);
		if (pNewWorld->PreLoad(worldName))
		{
			return pNewWorld;
		}

		// Return nullptr and display error
		m_pApplication->GetLoggingSystem().Log(LOGLEVEL_ERROR, "World", "Couldn't open world with path: " + worldName);
		delete pNewWorld;
		pNewWorld = nullptr;

		return pNewWorld;

	}, [this, bReplace](World* pWorld)
	{
		if (pWorld == nullptr)
			return;

		if (bReplace)
			m_pApplication->GetWorldManager().LoadWorld(pWorld);
		else
		{
			// Pop current world if we want to load the previous world without replacing
			size_t worldCount = m_pApplication->GetWorldManager().GetWorldCount();
			if (!bReplace && worldCount > 1 && m_pApplication->GetWorldManager().GetWorlds()[worldCount - 2] == pWorld)
				m_pApplication->GetWorldManager().PopWorld();
			else m_pApplication->GetWorldManager().PushWorld(pWorld);
		}
	});
}

JobQueue* World::GetJobQueue()
{
	return m_pApplication->GetJobManager().GetJobQueue(m_JobQueueHandle);
}

void World::SetPhysicsSystem(PhysicsSystem * pNewPhysicsSystem)
{
	m_pPhysicsSystem = pNewPhysicsSystem;
}

void World::SetAudioSystem(AudioSystem * pNewAudioSystem)
{
	m_pAudioSystem = pNewAudioSystem;
}

void World::SetRenderSystem(RenderSystem * pNewRenderSystem)
{
	m_pRenderSystem = pNewRenderSystem;
}

void World::SetChunkSystem(ChunkSystem* pNewChunkSystem)
{
	m_pChunkSystem = pNewChunkSystem;
}

void World::DeleteEntityFromLists(Entity * pEntity)
{
	OPTICK_EVENT();
	const std::vector<Entity*>::iterator iter = std::find(m_Entities.begin(), m_Entities.end(), pEntity);
	const std::vector<Entity*>::iterator iterToAdd = std::find(m_AddedEntities.begin(), m_AddedEntities.end(), pEntity);

	if (iter != m_Entities.end())
		m_Entities.erase(iter);

	if (iterToAdd != m_AddedEntities.end())
		m_AddedEntities.erase(iterToAdd);

	EntityRemoved(pEntity);
}

std::vector<Entity*>& World::GetRemovedEntities()
{
	return m_RemovedEntities;
}

void World::DeleteComponentFromEntityLists(Entity * pEntity, Component * pComponent)
{
	OPTICK_EVENT();
	if (pEntity == nullptr || pComponent == nullptr)
		return;

	const std::vector<Component*>::iterator iter = std::find(pEntity->m_Components.begin(), pEntity->m_Components.end(), pComponent);
	const std::vector<Component*>::iterator iterToAdd = std::find(pEntity->m_AddedComponents.begin(), pEntity->m_AddedComponents.end(), pComponent);

	if (iter != pEntity->m_Components.end())
		pEntity->m_Components.erase(iter);

	if (iterToAdd != pEntity->m_AddedComponents.end())
		pEntity->m_AddedComponents.erase(iterToAdd);
}

std::vector<Component*>& World::GetRemovedComponentsFromEntity(Entity * pEntity)
{
	return pEntity->m_RemovedComponents;
}

void World::AddRemovedEntityToWorld(Entity * pEntity)
{
	OPTICK_EVENT();
	if (pEntity->GetWorld() == nullptr)
		pEntity->m_pWorld = this;

	AddEntity(pEntity);
}

Entity* World::RemoveEntityFromWorld(Entity * pEntity)
{
	OPTICK_EVENT();
	if (RemoveEntityChildsFromWorld(pEntity))
	{
		if (pEntity->m_pParent)
			pEntity->m_pParent->RemoveChild(pEntity);

		return pEntity;
	}

	return nullptr;
}

bool World::RemoveEntityChildsFromWorld(Entity * pEntityChild)
{
	OPTICK_EVENT();
	if (pEntityChild == nullptr || pEntityChild->m_bIsDestroyed)
		return false;

	pEntityChild->Destroyed(pEntityChild);

	for (Component* pEntityChildComponent : pEntityChild->m_Components)
		pEntityChildComponent->Destroyed(pEntityChildComponent);

	for (Entity* pEntity : pEntityChild->m_Children)
		RemoveEntityChildsFromWorld(pEntity);

	m_RemovedEntities.push_back(pEntityChild);

	return true;
}
void World::DeleteEntity(Entity* pEntity)
{
	DeleteEntityFromLists(pEntity);

	delete pEntity;
}