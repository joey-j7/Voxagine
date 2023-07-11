#pragma once

#include "Core/GameTimer.h"

template<typename T>
class FSMState {
public:
	FSMState() = default;

	virtual ~FSMState() = default;

	virtual void Awake(T*) {}
	virtual void Start(T*) {}

	/*!
	 * @param pOwner
	 * @param fDeltaTime
	 */
	virtual void Tick(T* pOwner, float fDeltaTime) = 0;

	virtual void FixedTick(T* pOwner, const GameTimer& gameTimer) {};

	virtual void Exit(T*) {}
};