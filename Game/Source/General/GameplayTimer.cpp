//#include "GameplayTimer.h"
//
//GameplayTimer::GameplayTimer(Entity* pOwner)
//{
//	m_pOwner = pOwner;
//}
//
//void GameplayTimer::Configure(
//	float fDuration,
//	const std::function<void>& finishFunction,
//
//	bool bLoop,
//	const std::function<void>& startFunction
//)
//{
//	m_fDuration = fDuration;
//	m_fCurrent = fDuration;
//
//	m_bLoop = bLoop;
//
//	m_OnStart(m_pOwner);
//}
//
//void GameplayTimer::Tick(float fDeltaTime)
//{
//	if (!m_bActive)
//		return;
//
//	if (m_fCurrent > 0.f)
//	{
//		m_fCurrent -= fDeltaTime;
//
//		if (m_fCurrent <= 0.f)
//		{
//			m_OnFinish(m_pOwner);
//
//			if (m_bLoop)
//			{
//				m_fCurrent = m_fDuration - std::fmod(m_fCurrent, m_fDuration);
//				m_OnStart(m_pOwner);
//			}
//			else
//			{
//				m_fCurrent = 0.f;
//				m_bActive = false;
//			}
//		}
//	}
//}
