#pragma once
#include "World.h"

class Component;
class ComponentSystem
{
public:
	ComponentSystem(World* pWorld);
	virtual ~ComponentSystem() {}

	void AddComponent(Component* pComponent);

	virtual void Start() {}
	virtual bool CanProcessComponent(Component* pComponent) = 0;
	virtual void Tick(float fDeltaTime) {}
	virtual void PostTick(float fDeltaTime) {}

	virtual void FixedTick(const GameTimer& fixedTimer) {}
	virtual void PostFixedTick(const GameTimer& fixedTimer) {}

	virtual void OnDrawGizmos(float fDeltaTime) {}

protected:
	virtual void OnComponentAdded(Component* pComponent) = 0;
	virtual void OnComponentDestroyed(Component* pComponent) = 0;

	World* m_pWorld;
};