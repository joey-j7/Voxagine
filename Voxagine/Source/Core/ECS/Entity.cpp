#include "pch.h"
#include "Core/ECS/Entity.h"

#include "Core/ECS/World.h"
#include "Core/ECS/Component.h"
#include "Core/ECS/Components/Transform.h"
#include "External/optick/optick.h"

#include "Core/ECS/Components/Collider.h"

#include <string>

#include <External/rttr/registration>
#include <Core/MetaData/PropertyTypeMetaData.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<Entity>("Entity")
		.constructor<World*>()(rttr::policy::ctor::as_raw_ptr)
		.property("ID", &Entity::m_uiId) ( RTTR_PRIVATE )
		.property("Name", &Entity::GetName, &Entity::SetName) ( RTTR_PUBLIC )
		.property("Enabled", &Entity::IsEntityOnlyEnabled, &Entity::SetEnabled) ( RTTR_PUBLIC )
		.property("Static", &Entity::IsStatic, &Entity::SetStatic) ( RTTR_PUBLIC )
		.property("Destructible", &Entity::IsDestructible, &Entity::SetDestructible) (RTTR_PUBLIC)
		.property("Tags", &Entity::m_Tags) (RTTR_PUBLIC)
		.property("Persistent", &Entity::m_bIsPersistent) (RTTR_PUBLIC);
}

uint64_t Entity::EntityIdCounter = 1;

Entity::Entity(World * pWorld)
{
	m_pWorld = pWorld;
	m_pParent = nullptr;
	m_bIsDestroyed = false;
	m_bIsEnabled = true;
	m_bIsStatic = false;
	m_bIsDestructible = false;
	m_bIsInitialized = false;
	m_bIsPersistent = false;
	m_bIsAwake = false;
	m_uiId = EntityIdCounter++;
	m_Name = "Entity" + std::to_string(m_uiId);

	m_pTransform = new Transform(this);
	m_Components.push_back(m_pTransform);
}

Entity::~Entity()
{
	for (Component* pComponent : m_Components)
	{
		delete pComponent;
		pComponent = nullptr;
	}

	for (Component* pComponent : m_AddedComponents)
	{
		delete pComponent;
		pComponent = nullptr;
	}
}

void Entity::Start()
{
	OPTICK_EVENT();
	m_bIsInitialized = true;
	for (Component* pComponent : m_Components)
	{
		if (!pComponent->IsInitialized())
		{
			pComponent->m_bIsInitialized = true;
			pComponent->Start();
		}
	}
	Initialized(this);
}

void Entity::PreTick()
{
	OPTICK_EVENT();
	m_pTransform->SetUpdated(false);
	m_pTransform->UpdateMatrix();

	/* Update the component lists */
	UpdateComponents();

	/* Register the components to the systems */
	m_pWorld->RegisterComponents(m_PreTickAddedComponents);

	if (!m_bIsAwake)
	{
		Awake();
		m_bIsAwake = true;
	}
}

void Entity::Tick(float fDeltaTime)
{
	OPTICK_CATEGORY("Gameplay", Optick::Category::GameLogic);
	OPTICK_EVENT();
	/* Run start method on entity and components when needed */
	if (!m_bIsInitialized)
		Start();

	/* Run start methods on newly added components when needed */
	if (!m_PreTickAddedComponents.empty())
	{
		for (Component* pComponent : m_PreTickAddedComponents)
		{
			if (!pComponent->IsInitialized())
			{
				pComponent->m_bIsInitialized = true;
				pComponent->Start();
			}
		}
	}
}

void Entity::Destroy()
{
	if (!m_bIsDestroyed)
		m_pWorld->RemoveEntity(this);
}

void Entity::UpdateComponents()
{
	OPTICK_EVENT();
	/* Delete components queued for deletion */
	for (Component* pComponent : m_RemovedComponents)
	{
		const std::vector<Component*>::iterator iter = std::find(m_Components.begin(), m_Components.end(), pComponent);
		if (iter != m_Components.end())
		{
			for (Component* pAliveComp : m_Components)
			{
				if (pAliveComp == pComponent) continue;
				pAliveComp->DependencyCheck(pComponent, false);
			}

			m_Components.erase(iter);
			delete pComponent;
		}
	}
	m_RemovedComponents.clear();

	m_PreTickAddedComponents.clear();
	if (!m_AddedComponents.empty())
	{
		/* Add all queued components to the list */
		for (Component* pComponent : m_AddedComponents)
			m_Components.push_back(pComponent);

		/* Call Awake when the new components have been added */
		for (Component* pComponent : m_AddedComponents)
		{
			pComponent->DependencyCheck(pComponent, true);
			pComponent->Awake();
		}

		/* Remember the added components for Tick */
		m_PreTickAddedComponents = std::vector<Component*>(m_AddedComponents);
		m_AddedComponents.clear();
	}
}

void Entity::DeInitialize()
{
	OPTICK_EVENT();
	if (m_bIsDestroyed) return;
	m_bIsDestroyed = true;

	Destroyed(this);

	if (m_pParent)
	{
		m_pParent->RemoveChild(this);
		m_pParent = nullptr;
	}

	for (Component* pComponent : m_Components)
		pComponent->Destroyed(pComponent);

	while (m_Children.size() > 0)
	{
		m_Children[0]->Destroy();
	}
}

Entity* Entity::GetChild(std::string name)
{
	for (Entity* pEntity : m_Children)
	{
		if (pEntity->GetName() == name)
			return pEntity;
	}
	return nullptr;
}

Entity* Entity::GetChild(uint64_t uiId)
{
	for (Entity* pEntity : m_Children)
	{
		if (pEntity->GetId() == uiId)
			return pEntity;
	}
	return nullptr;
}

std::vector<Entity*> Entity::GetChildren(std::string name)
{
	std::vector<Entity*> children;
	for (Entity* pEntity : m_Children)
	{
		if (pEntity->GetName() == name)
			children.push_back(pEntity);
	}
	return children;
}

void Entity::AddChild(Entity* pEntity)
{
	pEntity->SetParent(this);
}

void Entity::RemoveChild(Entity * pEntity)
{
	std::vector<Entity*>::iterator iter = std::find(m_Children.begin(), m_Children.end(), pEntity);
	if (iter != m_Children.end())
		m_Children.erase(iter);
}

void Entity::SetParent(Entity* pEntity)
{
	OPTICK_EVENT();
	if (m_pParent != nullptr)
	{
		m_pParent->RemoveChild(this);
	}

	m_pParent = pEntity;

	if (pEntity != nullptr)
	{
		Entity* LastParrent = pEntity;
		Entity* CurrentParent = pEntity->GetParent();

		while (CurrentParent != nullptr)
		{
			if (CurrentParent == this)
			{
				RemoveChild(LastParrent);
				LastParrent->SetParent(GetParent());
			}

			LastParrent = CurrentParent;
			CurrentParent = CurrentParent->GetParent();
		}

		pEntity->m_Children.push_back(this);
	}

	GetTransform()->SetFromMatrix(GetTransform()->GetMatrix());
}

Entity * Entity::GetParentRoot()
{
	Entity* ParentRoot = nullptr;
	Entity* CurrentParent = GetParent();

	while (CurrentParent != nullptr)
	{
		ParentRoot = CurrentParent;
		CurrentParent = CurrentParent->GetParent();
	}

	return ParentRoot;
}

bool Entity::IsEnabled(bool bWithParent) const
{
	if (bWithParent && m_pParent && !m_pParent->IsEnabled())
	{
		return false;
	}

	return m_bIsEnabled;
}

void Entity::SetEnabled(bool bIsEnabled)
{
	if (bIsEnabled && !m_bIsEnabled)
	{
		OnEnabled();
	}
	else if (!bIsEnabled && m_bIsEnabled)
	{
		OnDisabled();
	}

	m_bIsEnabled = bIsEnabled;
}

void Entity::SetDestructible(bool bDestructible)
{
	m_bIsDestructible = bDestructible;

	auto pCollider = GetComponent<Collider>();
	if (pCollider && m_bIsDestructible)
	{
		pCollider->SetVoxelPreciseCollision(true);
	}
	
}

bool Entity::HasTag(std::string tag)
{
	std::vector<std::string>::iterator iter = std::find(m_Tags.begin(), m_Tags.end(), tag);
	return iter != m_Tags.end();
}

void Entity::AddTag(std::string tag)
{
	std::vector<std::string>::iterator iter = std::find(m_Tags.begin(), m_Tags.end(), tag);
	if (iter == m_Tags.end())
		m_Tags.push_back(tag);
}

bool Entity::RemoveTag(std::string tag)
{
	return std::remove_if(m_Tags.begin(), m_Tags.end(), [tag](const std::string& mTag) { return mTag == tag; }) != m_Tags.end();
}

std::vector<Component*> Entity::GetComponentsWithTag(std::string tag)
{
	std::vector<Component*> tagComps;
	for (Component* pComp : m_Components)
	{
		if (pComp->HasTag(tag))
			tagComps.push_back(pComp);
	}

	return tagComps;
}

Component* Entity::AddComponent(Component* pComponent)
{
	OPTICK_EVENT();
	auto searchFunc = [pComponent](Component* pComp)
	{
		return pComp->get_type() == pComponent->get_type();
	};

	if (std::find_if(m_Components.begin(), m_Components.end(), searchFunc) == m_Components.end() &&
		std::find_if(m_AddedComponents.begin(), m_AddedComponents.end(), searchFunc) == m_AddedComponents.end())
	{
		m_AddedComponents.push_back(pComponent);
		return pComponent;
	}
	else delete pComponent;
	return nullptr;
}

void Entity::RemoveComponent(Component* pComponent)
{
	OPTICK_EVENT();
	std::vector<Component*>::iterator iter = std::find(m_Components.begin(), m_Components.end(), pComponent);
	if (iter != m_Components.end())
	{
		pComponent->Destroyed(pComponent);
		m_RemovedComponents.push_back(pComponent);
	}
}

Component* Entity::GetComponent(const rttr::type& rType)
{
	for (const auto& pComponent : m_Components)
	{
		// sometimes it will do a check like Component == Component*
		if (Utils::CheckDerivedType(pComponent->get_type(), rType))
			return pComponent;
	}

	return nullptr;
}
