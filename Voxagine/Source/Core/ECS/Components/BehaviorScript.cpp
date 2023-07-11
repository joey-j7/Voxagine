#include "pch.h"
#include "BehaviorScript.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<BehaviorScript>("BehaviorScript")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr);
}

BehaviorScript::BehaviorScript(Entity * pEntity) :
	Component(pEntity)
{
}

BehaviorScript::~BehaviorScript()
{
	for (Delegate* pDelegate : m_Delegates)
		delete pDelegate;

	for (Routine* pRoutine : m_Routines)
		delete pRoutine;
}

void BehaviorScript::StartRoutine(std::function<double()> func, std::string routineName)
{
	m_Routines.push_back(new Routine(func, routineName));
}

void BehaviorScript::StopRoutine(std::string routineName)
{
	for (Routine* pRoutine : m_Routines)
	{
		if (pRoutine->RoutineName == routineName)
		{
			m_Routines.erase(std::remove(m_Routines.begin(), m_Routines.end(), pRoutine), m_Routines.end());
			delete pRoutine;
		}
	}
}

void BehaviorScript::Invoke(std::function<void()> func, double dDelay)
{
	m_Delegates.push_back(new Delegate(func, dDelay));
}
