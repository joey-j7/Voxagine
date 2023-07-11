#include "pch.h"
#include "ComponentSystem.h"

#include "Component.h"

ComponentSystem::ComponentSystem(World* pWorld)
{
	m_pWorld = pWorld;
}

void ComponentSystem::AddComponent(Component* pComponent)
{
	OnComponentAdded(pComponent);
	pComponent->Destroyed += Event<Component*>::Subscriber(std::bind(&ComponentSystem::OnComponentDestroyed, this, std::placeholders::_1), this);
}
