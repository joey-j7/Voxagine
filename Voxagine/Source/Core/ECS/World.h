#pragma once
#include <vector>
#include "Core/Event.h"
#include "Core/ECS/Components/Transform.h"
#include "Core/Objects/TSubclass.h"
#include "Core/Math.h"
#include "Core/Threading/JobManager.h"

class ChunkSystem;
class Entity;
class ComponentSystem;
class Application;
class Component;
class RenderSystem;
class AudioSystem;
class VoxelGrid;
class Camera;
class PhysicsSystem;
struct HitResult;
class DebugRenderer;
class GameTimer;
class World
{
public:
	friend class JsonSerializer;

	World(Application* pApp);
	virtual ~World();

	Event<Entity*> EntityAdded;
	Event<Entity*> EntityRemoved;
	Event<World*> Paused;
	Event<World*> Resumed;

	/* Setup functions */
	virtual void Initialize();
	bool PreLoad(const std::string& filePath);
	void PreLoad();
	virtual void Unload();

	/* Empty functions used in the future */
	void Pause();
	void Resume();

	/* Processes the add and remove queues for entities and components */
	virtual void PreTick();

	/* Processes Start and Tick functions on entities, components and systems */
	virtual void Tick(float fDeltaTime);

	/* Processes the FixedTick function for all entities and systems */
	virtual void FixedTick(const GameTimer& fixedTimer);

	/* Processes the PostFixedTick function for all entities and systems */
	virtual void PostFixedTick(const GameTimer& fixedTimer);

	/* Processes PostTick functions on entities and systems */
	virtual void PostTick(float fDeltaTime);

	/* Enables user to draw debug data anytime */
	void OnDrawGizmos(float fDeltaTime);

	/* Processes the Render function on the systems */
	void Render(const GameTimer& fixedTimer);

	/* Functions for registering entities, components and systems to the world */
	void RegisterComponent(Component* pComponent);
	void RegisterComponents(const std::vector<Component*>& components);

	/* Adds a single entity to the world, this does not include his children */
	void AddEntity(Entity* pEntity);

	/* Adds a entity to the world which also adds the entire child tree of this entity */
	void AddRootEntity(Entity* pRootEntity);

	void RemoveEntity(Entity* pEntity);
	void SetSystems(std::vector<ComponentSystem*> systems);

	template<typename T>
	T* SpawnEntity(Vector3 position, Vector3 rotation, Vector3 scale);

	template<typename T>
	T* SpawnEntity(Vector3 position, Quaternion rotation, Vector3 scale);

	template<typename T>
	TSubclass<T>& SpawnEntity(TSubclass<T>& subclass, Vector3 position, Quaternion rotation, Vector3 scale);

	Entity* SpawnEntity(rttr::type entityType, Vector3 position, Quaternion rotation, Vector3 scale);

	template<typename T>
	T* GetSystem();

	ChunkSystem* GetChunkSystem() const { return m_pChunkSystem; }
	RenderSystem* GetRenderSystem() const { return m_pRenderSystem; }
	PhysicsSystem* GetPhysics() const { return m_pPhysicsSystem; }
	DebugRenderer* GetDebugRenderer() const;

	template <typename T>
	std::vector<T*> FindEntitiesOfType();

	template<typename T>
	std::vector<Entity*> FindEntitiesWithComponent();

	/* Functions for searching entities in the world */
	Entity* FindEntity(std::string name);
	Entity* FindEntity(uint64_t uiId);
	//Searches all lists including pending add and pending kill entities
	Entity* FindEntityAll(uint64_t uiId);
	std::vector<Entity*> FindEntities(std::string name);
	std::vector<Entity*> FindEntitiesWithTag(std::string tag);
	bool FindEntityWithTag(std::string tag);
	const std::vector<Entity*>& GetEntities();
	const std::vector<Entity*>& GetAddedEntities();

	/* Forwarding RayCast function to PhysicsSystem */
	bool RayCast(Vector3 start, Vector3 dir, HitResult& hitResult, float fLength = FLT_MAX, uint32_t uiLayer = -1);
	const VoxelGrid* GetVoxelGrid();
	void ApplySphericalDestruction(const Vector3& position, float fRadius, float fForceMin, float fForceMax, bool bBakeParticles = true);

	/* Forwarding time functions */
	float GetDeltaSeconds();
	float GetTotalSeconds();

	bool IsPreLoaded() const;
	Application* GetApplication() const;
	void SetMainCamera(Camera* pCamera);
	Camera* GetMainCamera() const { return m_pCameraEntity; }
	std::string GetName() const { return m_WorldName; }
	UVector2 GetWorldSize() const;

	const std::string& GetGroundTexturePath() const { return m_GroundTexturePath; };
	void SetGroundTexturePath(const std::string& texturePath);

	void SetWorldName(std::string name) { m_WorldName = name; }

	//Loads a new world
	//Set replace to true if the new worlds needs to replace the current world
	void OpenWorld(const std::string& worldName, bool bReplace = true);

	//Loads a new world asynchronously
	//Set replace to true if the new worlds needs to replace the current world
	void OpenWorldAsync(const std::string& worldName, bool bReplace = true);

	JobQueue* GetJobQueue();

protected:
	void SetPhysicsSystem(PhysicsSystem* pNewPhysicsSystem);
	void SetAudioSystem(AudioSystem * pNewAudioSystem);
	void SetRenderSystem(RenderSystem* pNewRenderSystem);
	void SetChunkSystem(ChunkSystem* pNewChunkSystem);

	std::vector<ComponentSystem*> m_Systems;
	RenderSystem* m_pRenderSystem;
	PhysicsSystem* m_pPhysicsSystem;
	AudioSystem* m_pAudioSystem;
	ChunkSystem* m_pChunkSystem;
	QueueHandle m_JobQueueHandle;

	void DeleteEntityFromLists(Entity* pEntity);
	std::vector<Entity*>& GetRemovedEntities();

	void DeleteComponentFromEntityLists(Entity* pEntity, Component* pComponent);
	std::vector<Component*>& GetRemovedComponentsFromEntity(Entity* pEntity);

	void AddRemovedEntityToWorld(Entity* pEntity);
	Entity* RemoveEntityFromWorld(Entity* pEntity);
	bool RemoveEntityChildsFromWorld(Entity* pEntityChild);

private:
	/**
	 * @brief EntityConnection - a struct that defines information about the connected
	 * that needs to be made.
	 */
	struct WorldConnectionInformation
	{
		rttr::instance rInstance;
		rttr::property rProperty;
		rttr::type rType = rttr::type::get<nullptr_t>();
		int64_t iEntityId = -1;
		int iIndex = -1;
	};

	Application* m_pApplication;
	std::vector<Entity*> m_Entities;
	std::vector<Entity*> m_AddedEntities;
	std::vector<Entity*> m_RemovedEntities;
	Camera* m_pCameraEntity;

	std::string m_WorldName;
	bool m_bPreLoaded;

	std::string m_GroundTexturePath = "";

	std::vector<WorldConnectionInformation> m_vWorldConnections = {};

	void DeleteEntity(Entity* pEntity);
};

template<typename T>
T* World::SpawnEntity(Vector3 position, Vector3 rotation, Vector3 scale)
{
	Quaternion q = Quaternion(Vector3(rotation.x, rotation.y, rotation.z));
	return SpawnEntity<T>(position, q, scale);
}

template<typename T>
T* World::SpawnEntity(Vector3 position, Quaternion rotation, Vector3 scale)
{
	static_assert(std::is_base_of<Entity, T>::value, "Type must derive from Entity");
	T* entity = new T(this);
	Transform* transform = entity->GetTransform();
	transform->SetPosition(position);
	transform->SetRotation(rotation);
	transform->SetScale(scale);
	AddEntity(entity);

	return entity;
}

template<typename T>
TSubclass<T>& World::SpawnEntity(TSubclass<T>& subclass, Vector3 position, Quaternion rotation, Vector3 scale)
{
	static_assert(std::is_base_of<Entity, T>::value, "Type must derive from Entity");

	if(rttr::type::get<Entity*>().is_base_of(subclass.get_derived_type()) && subclass.get_derived_type().is_valid())
	{
		rttr::variant entityVar = subclass.get_derived_type().create({ this });
		subclass.m_pClassReference = entityVar.get_value<T*>();
		Transform* transform = subclass.m_pClassReference->GetTransform();
		transform->SetPosition(position);
		transform->SetRotation(rotation);
		transform->SetScale(scale);
		AddEntity(subclass.m_pClassReference);
	}

	return subclass;
}

template<typename T>
T* World::GetSystem()
{
	for (ComponentSystem* pSystem : m_Systems)
	{
		if (T* pConvertedSystem = dynamic_cast<T*>(pSystem))
			return pConvertedSystem;
	}
	return nullptr;
}

template <typename T>
std::vector<Entity*> World::FindEntitiesWithComponent()
{
	static_assert(std::is_base_of<Component, T>::value, "Type must derive from Component");

	// result
	std::vector<Entity*> result = {};
 
	for (Entity* pEntity : m_Entities)
	{
		if (pEntity->GetComponent<T>())
			result.push_back(pEntity);
	}
	return result;
}

template <typename T>
std::vector<T*> World::FindEntitiesOfType()
{
	static_assert(std::is_base_of<Entity, T>::value, "Type must derive from Entity");
	
	std::vector<T*> entities;
	rttr::type entityType = rttr::type::get<T>();
	for (Entity* pEntity : m_Entities)
	{
		if (pEntity->get_type() == entityType)
			entities.push_back(static_cast<T*>(pEntity));
	}
	return entities;
}
