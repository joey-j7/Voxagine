#pragma once
#pragma warning(push, 0) 
#include <External/moodycamel/concurrentqueue.h>
#pragma warning(pop)

#include "Core/Threading/Job.h"
#include "Core/Threading/GenericJob.h"
#include "Core/Threading/JobThread.h"
#include "Core/Threading/JobQueue.h"
#include <vector>
#include <unordered_map>
#include <atomic>
#include <shared_mutex>

typedef uint32_t QueueHandle;

class JobManager
{
public:
	friend class Application;
	friend class PhysicsSystem;
	friend class WINFileSystem;
	friend class ORBFileSystem;
	friend class JobQueue;
	friend class Job;

	JobManager();
	~JobManager();

	QueueHandle CreateJobQueue();
	JobQueue* GetJobQueue(QueueHandle handle);
	void DiscardJobQueue(QueueHandle handle);
	void ShelveJobQueue(QueueHandle handle);
	void UnShelveJobQueue(QueueHandle handle);

private:
	moodycamel::ConcurrentQueue<Job*> m_FinishedJobQueue;
	std::vector<JobThread*> m_WorkerThreads;

	uint32_t m_uiNumActiveThreads = 0;
	uint32_t m_uiMaxNumThreads = 0;
	uint32_t m_uiThreadSleepTime = 10;
	std::atomic_bool m_bKillThreads = { false };

	void Initialize();
	void Deinitialize();

	void ProcessFinishedJobs();
	void WaitForJob(Job* pJob);

	void ThreadLoop(JobThread* pThrea);

private:
	std::shared_timed_mutex m_JobMutex;
	uint32_t m_QueueHandleCtr = 0;
	std::unordered_map<QueueHandle, JobQueue*> m_JobQueues;
	std::unordered_map<QueueHandle, JobQueue*> m_ShelvedJobQueues;
};
