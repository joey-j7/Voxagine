#pragma once
#pragma warning(push, 0) 
#include <External/moodycamel/concurrentqueue.h>
#pragma warning(pop)

#include "Core/Threading/Job.h"
#include "Core/Threading/GenericJob.h"

class JobQueue 
{
public:
	friend class JobManager;
	friend class PhysicsSystem;
	friend class WINFileSystem;
	friend class ORBFileSystem;

	JobQueue(JobManager* pJobManager, uint32_t handle);
	~JobQueue();

	template<typename T>
	Job* Enqueue(std::function<T()> backgroundFunc, std::function<void(T)> callback);

	void Enqueue(Job* pJob);
	void EnqueueBulk(const std::vector<Job*>& pJobs);

private:
	std::unordered_map<JobType, moodycamel::ConcurrentQueue<Job*>> m_JobQueue;
	JobManager* m_pJobManager = nullptr;
	uint32_t m_QueueHandle = UINT_MAX;

	void EnqueueWithType(Job* pJob, JobType jobType);

	template<typename T>
	Job* EnqueueWithType(std::function<T()> backgroundFunc, std::function<void(T)> callback, JobType jobType);
};

template<typename T>
Job* JobQueue::Enqueue(std::function<T()> backgroundFunc, std::function<void(T)> callback)
{
	Job* pJob = new GenericJob<T>(backgroundFunc, callback);
	pJob->SetJobManager(m_pJobManager);
	pJob->SetQueueHandle(m_QueueHandle);
	m_JobQueue[JT_DEFAULT].enqueue(pJob);
	return pJob;
}

template<typename T>
Job* JobQueue::EnqueueWithType(std::function<T()> backgroundFunc, std::function<void(T)> callback, JobType jobType)
{
	Job* pJob = new GenericJob<T>(backgroundFunc, callback);
	pJob->SetJobManager(m_pJobManager);
	pJob->SetQueueHandle(m_QueueHandle);
	pJob->m_Type = jobType;
	m_JobQueue[jobType].enqueue(pJob);
	return pJob;
}
