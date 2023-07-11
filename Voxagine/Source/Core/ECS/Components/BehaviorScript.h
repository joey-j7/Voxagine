#pragma once
#include "Core/ECS/Component.h"

#include <External/rttr/type>

#define STOP_ROUTINE -1

class Collider;
struct Manifold;
class BehaviorScript : public Component
{
public:
	friend class ScriptSystem;

	struct Delegate
	{
		Delegate(std::function<void()> function, double fDelay)
		{
			Function = function;
			Timer = fDelay;
		}

		std::function<void()> Function;
		double Timer;
	};

	struct Routine
	{
		Routine(std::function<double()> function, std::string routineName)
		{
			Timer = 0.f;
			WaitAmount = 0.f;
			Function = function;
			RoutineName = routineName;
		}

		double Timer;
		double WaitAmount;
		std::function<double()> Function;
		std::string RoutineName;
	};

	BehaviorScript(Entity* pEntity);
	virtual ~BehaviorScript();

	virtual void Tick(float fDeltaTime) {}
	virtual void PostTick(float fDeltaTime) {}
	virtual void FixedTick(const GameTimer&) {}
	virtual void PostFixedTick(const GameTimer&) {}

	virtual void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) {}
	virtual void OnCollisionStay(Collider* pCollider, const Manifold& manifold) {}
	virtual void OnCollisionExit(Collider* pCollider, const Manifold& manifold) {}
	
	void StartRoutine(std::function<double()> func, std::string routineName);
	void StopRoutine(std::string routineName);
	void Invoke(std::function<void()> func, double dDelay);

private:
	std::vector<Delegate*> m_Delegates;
	std::vector<Routine*> m_Routines;

	RTTR_ENABLE(Component)
};