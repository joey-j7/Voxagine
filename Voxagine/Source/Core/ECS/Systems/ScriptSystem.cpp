#include "pch.h"
#include "Core/ECS/Systems/ScriptSystem.h"

#include "Core/ECS/Components/Collider.h"
#include "External/optick/optick.h"

ScriptSystem::ScriptSystem(World* pWorld) :
	ComponentSystem(pWorld)
{
}

ScriptSystem::~ScriptSystem()
{
}

bool ScriptSystem::CanProcessComponent(Component* pComponent)
{
	return dynamic_cast<BehaviorScript*>(pComponent);
}

void ScriptSystem::Tick(float fDeltaTime)
{
	OPTICK_CATEGORY("Gameplay", Optick::Category::GameLogic);
	OPTICK_EVENT();
	for (BehaviorScript* pScript : m_Scripts)
	{
		if (!pScript->IsEnabled()) continue;

		pScript->Tick(fDeltaTime);

		UpdateRoutines(pScript, fDeltaTime);
		UpdateDelegates(pScript, fDeltaTime);
	}
}

void ScriptSystem::PostTick(float fDeltaTime)
{
	OPTICK_CATEGORY("Gameplay-Post", Optick::Category::GameLogic);
	OPTICK_EVENT();
	for (BehaviorScript* pScript : m_Scripts)
	{
		if (pScript->IsEnabled())
			pScript->PostTick(fDeltaTime);
	}
}

void ScriptSystem::FixedTick(const GameTimer& fixedTimer)
{
	OPTICK_CATEGORY("Gameplay-Fixed", Optick::Category::GameLogic);
	OPTICK_EVENT();
	for (BehaviorScript* pScript : m_Scripts)
	{
		if (pScript->IsEnabled())
			pScript->FixedTick(fixedTimer);
	}
}

void ScriptSystem::PostFixedTick(const GameTimer& fixedTimer)
{
	OPTICK_EVENT();
	for (BehaviorScript* pScript : m_Scripts)
	{
		if (pScript->IsEnabled())
			pScript->PostFixedTick(fixedTimer);
	}
}

void ScriptSystem::OnComponentAdded(Component* pComponent)
{
	BehaviorScript* pScript = static_cast<BehaviorScript*>(pComponent);
	m_Scripts.push_back(pScript);

	Collider* pCollider = pScript->GetOwner()->GetComponent<Collider>();
	if (pCollider == nullptr) return;

	pCollider->CollisionEnter += Event<Collider*, const Manifold&>::Subscriber(
		std::bind(&BehaviorScript::OnCollisionEnter, pScript, std::placeholders::_1, std::placeholders::_2), pScript);

	pCollider->CollisionStay += Event<Collider*, const Manifold&>::Subscriber(
		std::bind(&BehaviorScript::OnCollisionStay, pScript, std::placeholders::_1, std::placeholders::_2), pScript);

	pCollider->CollisionExit += Event<Collider*, const Manifold&>::Subscriber(
		std::bind(&BehaviorScript::OnCollisionExit, pScript, std::placeholders::_1, std::placeholders::_2), pScript);
}

void ScriptSystem::OnComponentDestroyed(Component* pComponent)
{
	BehaviorScript* pScript = static_cast<BehaviorScript*>(pComponent);
	const std::vector<BehaviorScript*>::iterator iter = std::find(m_Scripts.begin(), m_Scripts.end(), pScript);
	if (iter != m_Scripts.end())
		m_Scripts.erase(iter);
}

void ScriptSystem::UpdateRoutines(BehaviorScript* pScript, float fDeltaTime) const
{
	std::vector<BehaviorScript::Routine*>& routines = pScript->m_Routines;
	for (std::vector<BehaviorScript::Routine*>::iterator itRoutine = routines.begin(); itRoutine != routines.end();)
	{
		(*itRoutine)->Timer += fDeltaTime;
		if ((*itRoutine)->WaitAmount <= (*itRoutine)->Timer)
		{
			(*itRoutine)->Timer = 0;
			(*itRoutine)->WaitAmount = (*itRoutine)->Function();
			if ((*itRoutine)->WaitAmount == STOP_ROUTINE)
			{
				itRoutine = routines.erase(itRoutine);
				continue;
			}
		}
		++itRoutine;
	}
}

void ScriptSystem::UpdateDelegates(BehaviorScript* pScript, float fDeltaTime)
{
	std::vector<BehaviorScript::Delegate*>& delegates = pScript->m_Delegates;
	for (std::vector<BehaviorScript::Delegate*>::iterator iter = delegates.begin(); iter != delegates.end();)
	{
		(*iter)->Timer -= fDeltaTime;
		if ((*iter)->Timer <= 0)
		{
			(*iter)->Function();
			iter = delegates.erase(iter);
			continue;
		}
		++iter;
	}
}
