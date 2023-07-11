#pragma once
#include "Core/ECS/ComponentSystem.h"
#include "Core/ECS/Components/BehaviorScript.h"

class ScriptSystem : public ComponentSystem
{
public:
	ScriptSystem(World* pWorld);
	virtual ~ScriptSystem();

	virtual bool CanProcessComponent(Component* pComponent) override;
	virtual void Tick(float fDeltaTime) override;
	virtual void PostTick(float fDeltaTime) override;
	virtual void FixedTick(const GameTimer& fixedTimer) override;
	virtual void PostFixedTick(const GameTimer& fixedTimer) override;

protected:
	virtual void OnComponentAdded(Component* pComponent) override;
	virtual void OnComponentDestroyed(Component* pComponent) override;

	void UpdateRoutines(BehaviorScript* pScript, float fDeltaTime) const;
	void UpdateDelegates(BehaviorScript* pScript, float fDeltaTime);

private:
	std::vector<BehaviorScript*> m_Scripts;
};