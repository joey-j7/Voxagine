#pragma once
#include <vector>
#include "Core/Event.h"

#include <External/rttr/type>
#include <External/rttr/registration_friend> 

class World;
class Component;
class Transform;
class Collider;
class GameTimer;
struct Manifold;
struct Voxel;

class Entity
{
public:
	friend class World;
	friend class JsonSerializer;

	Entity(World* pWorld);
	virtual ~Entity();

	Event<Entity*> Destroyed;
	Event<Entity*> Initialized;
	Event<Entity*, bool> StaticPropertyChanged;

	virtual void Awake() {}
	virtual void Start();
	virtual void OnCollisionEnter(Collider*, const Manifold&) {};
	virtual void OnCollisionStay(Collider*, const Manifold&) {};
	virtual void OnCollisionExit(Collider*, const Manifold&) {};
	virtual void OnVoxelCollision(Voxel** voxels, uint32_t uiSize, bool& isHandled) {};

	virtual void OnEnabled() {};
	virtual void OnDisabled() {};

	virtual void PreTick();
	virtual void Tick(float fDeltaTime);
	virtual void FixedTick(const GameTimer&) {}
	virtual void PostFixedTick(const GameTimer&) {}
	virtual void PostTick(float fDeltaTime) {}
	virtual void OnDrawGizmos(float fDeltaTime) {}

	void Destroy();
	void UpdateComponents();

	Entity* GetChild(std::string name);
	Entity* GetChild(uint64_t uiId);
	std::vector<Entity*> GetChildren(std::string name);
	const std::vector<Entity*>& GetChildren() { return m_Children; }

	void AddChild(Entity* pEntity);
	void RemoveChild(Entity* pEntity);
	void SetParent(Entity* pEntity);
	Entity* GetParentRoot();

	World* GetWorld() const { return m_pWorld; }
	Transform* GetTransform() const { return m_pTransform; }
	Entity* GetParent() const { return m_pParent; }
	std::string GetName() const { return m_Name; }
	uint64_t GetId() const { return m_uiId; }
	bool IsDestroyed() const { return m_bIsDestroyed; }
	bool IsEnabled(bool bWithParent = true) const;
	bool IsEntityOnlyEnabled() const { return IsEnabled(false); };
	bool IsPersistent() const { return m_bIsPersistent; }
	void SetPersistent(bool bPersistent) { m_bIsPersistent = bPersistent; }
	void SetEnabled(bool bIsEnabled);
	bool IsInitialized() const { return m_bIsInitialized;  }

	bool IsStatic() const { return m_bIsStatic; }
	void SetStatic(bool bStatic) {
		if (m_bIsStatic != bStatic)
		{
			m_bIsStatic = bStatic;
			StaticPropertyChanged(this, bStatic);
		}
	}

	bool IsDestructible() const { return m_bIsDestructible; }
	void SetDestructible(bool bDestructible);

	void SetName(std::string name) { m_Name = name; }

	bool HasTag(std::string tag);
	bool RemoveTag(std::string tag);
	void AddTag(std::string tag);

	std::vector<Component*> GetComponentsWithTag(std::string tag);

	//Returns a reference of the tags vector
	std::vector<std::string>& GetTags() { return m_Tags; }
	
	template<typename T>
	T* AddComponent();

	Component* AddComponent(Component* pComponent);

	void RemoveComponent(Component* pComponent);

	template<typename T>
	void RemoveComponent();

	template<typename T>
	T* GetComponent();

	template<typename T>
	T* GetComponentAll();

	Component* GetComponent(const rttr::type& rType);

	const std::vector<Component*>& GetPreTickAddedComponents() const { return m_PreTickAddedComponents; }
	const std::vector<Component*>& GetAddedComponents() const { return m_AddedComponents; }
	const std::vector<Component*>& GetComponents() const { return m_Components; }

private:
	void DeInitialize();

	World* m_pWorld;
	Transform* m_pTransform;

	/* Components */
	std::vector<Component*> m_Components;
	std::vector<Component*> m_AddedComponents;
	std::vector<Component*> m_RemovedComponents;
	std::vector<Component*> m_PreTickAddedComponents;
	
	/* Entity parameters */
	bool m_bIsStatic;
	bool m_bIsDestructible;
	bool m_bIsEnabled;
	bool m_bIsDestroyed;
	bool m_bIsInitialized;
	bool m_bIsPersistent;
	bool m_bIsAwake;
	std::string m_Name;
	std::vector<std::string> m_Tags;
	uint64_t m_uiId;

	/* Parent - Child System */
	Entity* m_pParent;
	std::vector<Entity*> m_Children;

	static uint64_t EntityIdCounter;

	RTTR_ENABLE()
	RTTR_REGISTRATION_FRIEND
};

template<typename T>
T* Entity::AddComponent()
{
	static_assert(std::is_base_of<Component, T>::value, "T must be of type Component");
	T* pComponent = new T(this);
	return (T*)AddComponent(pComponent);
}

template<typename T>
void Entity::RemoveComponent()
{
	std::vector<Component*>::iterator iter = m_Components.begin();
	for (; iter != m_Components.end(); ++iter)
	{
		if (T* pConvertedComponent = dynamic_cast<T*>(*iter))
		{
			pConvertedComponent->Destroyed(pConvertedComponent);
			m_RemovedComponents.push_back(*iter);
			break;
		}
	}
}

template<typename T>
T* Entity::GetComponent()
{
	for (Component* pComponent : m_Components)
	{
		if (T* pConvertedComponent = dynamic_cast<T*>(pComponent))
			return pConvertedComponent;
	}
	return nullptr;
}

template<typename T>
T* Entity::GetComponentAll()
{
	for (Component* pComponent : m_Components)
	{
		if (T* pConvertedComponent = dynamic_cast<T*>(pComponent))
			return pConvertedComponent;
	}
	for (Component* pComponent : m_RemovedComponents)
	{
		if (T* pConvertedComponent = dynamic_cast<T*>(pComponent))
			return pConvertedComponent;
	}
	for (Component* pComponent : m_AddedComponents)
	{
		if (T* pConvertedComponent = dynamic_cast<T*>(pComponent))
			return pConvertedComponent;
	}
	return nullptr;
}
