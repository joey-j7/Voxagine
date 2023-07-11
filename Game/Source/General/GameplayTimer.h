//#pragma once
//
//#include <functional>
//
//#include "Core/ECS/Entity.h"
//
//class GameplayTimer
//{
//public:
//	GameplayTimer(Entity* pOwner);
//
//	void Configure(
//		float fDuration,
//
//		const std::function<void>& finishFunction,
//
//		bool bLoop = false,
//		const std::function<void>& startFunction = {}
//	);
//
//	void Tick(float fDeltaTime);
//
//	bool IsActive() const { return m_bActive; }
//
//	void Activate() { m_bActive = true; }
//	void Deativate() { m_bActive = false; }
//
//private:
//	float m_fDuration = 0.f;
//	float m_fCurrent = 0.f;
//
//	bool m_bActive = true;
//	bool m_bLoop = false;
//
//	Entity* m_pOwner = nullptr;
//
//	std::function<void(Entity*)> m_OnStart = {};
//	std::function<void(Entity*)> m_OnFinish = {};
//};