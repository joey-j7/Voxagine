#include "pch.h"
#include "JobThread.h"

JobThread::~JobThread()
{
	if (m_Thread->joinable())
		m_Thread->join();

	delete m_Thread;
}

Job* JobThread::GetRunningJob()
{
	std::shared_lock<std::shared_timed_mutex> lock(m_JobMutex);
	return m_pRunningJob;
}

void JobThread::SetRunningJob(Job* pJob)
{
	std::unique_lock<std::shared_timed_mutex> lock(m_JobMutex);
	m_pRunningJob = pJob;
}
