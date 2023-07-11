#pragma once
#include <thread>
#include "Core/Threading/Job.h"

#include <shared_mutex>

class JobThread
{
public:
	~JobThread();

	Job* GetRunningJob();
	std::thread* GetThread() { return m_Thread; }
	JobType GetJobType() { return m_ThreadJobType; }

	void SetRunningJob(Job* pJob);
	void SetJobType(JobType type) { m_ThreadJobType = type; }
	void SetThread(std::thread* jobThread) { m_Thread = jobThread; }

private:
	std::shared_timed_mutex m_JobMutex;

	std::thread* m_Thread = nullptr;
	Job* m_pRunningJob = nullptr;
	JobType m_ThreadJobType = JT_DEFAULT;
};
