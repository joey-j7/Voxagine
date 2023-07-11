#pragma once
#include <type_traits>
#include <cassert>

#include "Core/GameTimer.h"

class Entity;

template<typename T>
class FSMState;

template<typename T>
class FiniteStateMachine 
{

public:
	/*!
	 * @param pOwner Owner of the state machine
	 */
	FiniteStateMachine(T* pOwner);

	~FiniteStateMachine();

	/*!
	 * @brief - Get the current state
	 */
	FSMState<T>* GetCurrentState() { return m_pCurrentState; }

	/*!
	 * @brief - Set the current state of the entity
	 * 
	 * @param pNewState new state of the entity
	 * @param bInitialized - to initialize the state
	 */
	void SetCurrentState(FSMState<T>* pNewState, bool bInitialize = true)
	{
		assert(pNewState); // NewState can't be nullptr

		if (m_pCurrentState && m_pCurrentState != pNewState)
		{
			// Exit the current state
			m_pCurrentState->Exit(m_pOwner);
		}
		// Set the current state with a new state
		m_pCurrentState = pNewState;

		if(bInitialize)
			// Start the current state
			m_pCurrentState->Start(m_pOwner);
	}

	void Start();

	void Tick(float fDeltaTime);
	void FixedTick(const GameTimer& fDeltaTime);

private:
	FSMState<T>* m_pCurrentState;

	T* m_pOwner;
};

//----------------------------------------------------------------------------
// Implementations
//----------------------------------------------------------------------------
template<typename T>
FiniteStateMachine<T>::FiniteStateMachine(T* pOwner) {
	static_assert(std::is_base_of<Entity, T>::value, "Value must be of type Entity");
	m_pCurrentState = nullptr;
	m_pOwner = pOwner;
}

template<typename T>
FiniteStateMachine<T>::~FiniteStateMachine() {
	// Entity will probably be destroyed by the world
	m_pOwner = nullptr;
}

template <typename T>
void FiniteStateMachine<T>::Start() 
{
	if(m_pCurrentState)
		m_pCurrentState->Start(m_pOwner);

}

template<typename T>
void FiniteStateMachine<T>::Tick(float fDeltaTime) 
{
	if (m_pCurrentState)
		m_pCurrentState->Tick(m_pOwner, fDeltaTime);
}

template<typename T>
void FiniteStateMachine<T>::FixedTick(const GameTimer& fDeltaTime) {
	if (m_pCurrentState)
		m_pCurrentState->FixedTick(m_pOwner, fDeltaTime);
}

