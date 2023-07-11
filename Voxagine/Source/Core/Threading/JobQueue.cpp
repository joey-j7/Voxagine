#include "pch.h"
#include "Core/Threading/JobQueue.h"

JobQueue::JobQueue(JobManager* pJobManager, uint32_t handle)
{
	m_pJobManager = pJobManager;
	m_QueueHandle = handle;

	m_JobQueue[JobType::JT_DEFAULT] = moodycamel::ConcurrentQueue<Job*>();
	m_JobQueue[JobType::JT_PHYSICS] = moodycamel::ConcurrentQueue<Job*>();
	m_JobQueue[JobType::JT_IO] = moodycamel::ConcurrentQueue<Job*>();
}

JobQueue::~JobQueue()
{
	for (auto& jobQueueIter : m_JobQueue)
	{
		Job* pJob = nullptr;
		while (jobQueueIter.second.try_dequeue(pJob))
		{
			delete pJob;
		}
	}
}

void JobQueue::Enqueue(Job* pJob)
{
	pJob->SetJobManager(m_pJobManager);
	pJob->SetQueueHandle(m_QueueHandle);
	m_JobQueue[JT_DEFAULT].enqueue(pJob);
}

void JobQueue::EnqueueBulk(const std::vector<Job*>& pJobs)
{
	for (auto& job : pJobs)
	{
		job->SetJobManager(m_pJobManager);
		job->SetQueueHandle(m_QueueHandle);
	}
		
	m_JobQueue[JT_DEFAULT].enqueue_bulk(pJobs.begin(), pJobs.size());
}

void JobQueue::EnqueueWithType(Job* pJob, JobType jobType)
{
	pJob->m_Type = jobType;
	pJob->SetJobManager(m_pJobManager);
	pJob->SetQueueHandle(m_QueueHandle);
	m_JobQueue[jobType].enqueue(pJob);
}

