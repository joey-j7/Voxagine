#pragma once

#include "FiniteStateMachine.h"

template<typename T>
class FSMState;

template<typename T>
class IFiniteManager
{
public:
	IFiniteManager() = default;

	virtual ~IFiniteManager()
	{
		for (auto state : m_mStates)
			delete state.second; //TODO: Fix warning: "deletion of pointer to incomplete type 'FSMState<T>'; no destructor called"


		m_mStates.clear();
	}

	void AddState(const std::pair<std::string, FSMState<T>*>& pairState, bool bSetCurrent = false)
	{
		auto stateIt = m_mStates.find(pairState.first);
		assert(stateIt == m_mStates.end() && "You added a duplicate to the state list");
		m_mStates.insert(pairState);

		if (bSetCurrent)
			SetState(pairState.first, false); // don't initialize
	}

	FSMState<T>* GetState(const std::string& sName) const { return m_mStates.at(sName); }
	void SetState(const std::string& sName, bool bInitialize = true)
	{
		auto stateIt = m_mStates.find(sName);
		if (stateIt != m_mStates.end())
			m_pFiniteStateMachine->SetCurrentState(m_mStates.at(sName), bInitialize);
	}

	FiniteStateMachine<T>* GetFSM() const { return m_pFiniteStateMachine; }

protected:
	FiniteStateMachine<T>* m_pFiniteStateMachine = nullptr;

private:
	std::unordered_map<std::string, FSMState<T>*> m_mStates;
};
