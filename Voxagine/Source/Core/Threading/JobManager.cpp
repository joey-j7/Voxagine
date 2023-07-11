#include "pch.h"
#include "Core/Threading/JobManager.h"

#include <chrono>
#include <queue>
#include "External/optick/optick.h"

JobManager::JobManager()
{
	// All available threads minus the main thread
	m_uiMaxNumThreads = std::thread::hardware_concurrency() - 1;
}

JobManager::~JobManager()
{
	
}

void JobManager::Initialize()
{
	// We always need one thread for each type at least
	JobThread* pPhysicsThread = new JobThread();
	pPhysicsThread->SetJobType(JT_PHYSICS);
	pPhysicsThread->SetThread(new std::thread(&JobManager::ThreadLoop, this, pPhysicsThread));
	m_WorkerThreads.push_back(pPhysicsThread);

	JobThread* pIoThread = new JobThread();
	pIoThread->SetJobType(JT_IO);
	pIoThread->SetThread(new std::thread(&JobManager::ThreadLoop, this, pIoThread));
	m_WorkerThreads.push_back(pIoThread);

	JobThread* pDefaultThread = new JobThread();
	pDefaultThread->SetJobType(JT_DEFAULT);
	pDefaultThread->SetThread(new std::thread(&JobManager::ThreadLoop, this, pDefaultThread));
	m_WorkerThreads.push_back(pDefaultThread);

	m_uiNumActiveThreads += 3;

	// Use extra thread slots for default jobs
	if (m_uiMaxNumThreads - 3 > 0)
	{
		for (uint32_t i = 0; i < m_uiMaxNumThreads - 3; ++i)
		{
			JobThread* pExtraDefaultThread = new JobThread();
			pExtraDefaultThread->SetJobType(JT_DEFAULT);
			pExtraDefaultThread->SetThread(new std::thread(&JobManager::ThreadLoop, this, pExtraDefaultThread));
			m_WorkerThreads.push_back(pExtraDefaultThread);
			++m_uiNumActiveThreads;
		}
	}
}

void JobManager::Deinitialize()
{
	m_bKillThreads = true;

	for (auto& jobQueue : m_JobQueues)
	{
		delete jobQueue.second;
	}
	m_JobQueues.clear();

	for (JobThread* jobThread : m_WorkerThreads)
	{
		if (jobThread->GetRunningJob())
			jobThread->GetRunningJob()->Canceled();

		delete jobThread;

		--m_uiNumActiveThreads;
	}
	m_WorkerThreads.clear();

	ProcessFinishedJobs();
}

void JobManager::ProcessFinishedJobs()
{
	OPTICK_EVENT();
	Job* pJob = nullptr;
	while (m_FinishedJobQueue.try_dequeue(pJob))
	{
		pJob->Finish();
		delete pJob;
	}
}

void JobManager::WaitForJob(Job* pJob)
{
	std::queue<Job*> dequeuedJobs;
	while (true)
	{
		Job* pTempJob = nullptr;
		while (m_FinishedJobQueue.try_dequeue(pTempJob))
		{
			if (pTempJob == pJob)
			{
				pJob->Finish();
				delete pJob;

				while (!dequeuedJobs.empty())
				{
					Job* topJob = dequeuedJobs.front();
					dequeuedJobs.pop();
					m_FinishedJobQueue.enqueue(topJob);
				}

				return;
			}

			dequeuedJobs.push(pTempJob);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

JobQueue* JobManager::GetJobQueue(QueueHandle handle)
{
	auto jobQueueIter = m_JobQueues.find(handle);
	if (jobQueueIter != m_JobQueues.end())
		return jobQueueIter->second;
	return nullptr;
}

void JobManager::ThreadLoop(JobThread* pThread)
{
	while (!m_bKillThreads)
	{
		Job* newJob = nullptr;
		bool jobFound = false;
		std::shared_lock<std::shared_timed_mutex> lock(m_JobMutex);
		for (auto& jobQueueIter : m_JobQueues)
		{
			if (jobQueueIter.second->m_JobQueue[pThread->GetJobType()].try_dequeue(newJob))
			{
				jobFound = true;
				pThread->SetRunningJob(newJob);
				newJob->SetWaiting(false);
				newJob->Run();
				pThread->SetRunningJob(nullptr);
				m_FinishedJobQueue.enqueue(newJob);
				break;
			}
		}
		if (!jobFound)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(m_uiThreadSleepTime));
		}
	}
}

QueueHandle JobManager::CreateJobQueue()
{
	std::unique_lock<std::shared_timed_mutex> lock(m_JobMutex);
	QueueHandle handle = m_QueueHandleCtr++;
	m_JobQueues[handle] = new JobQueue(this, handle);
	return handle;
}

void JobManager::DiscardJobQueue(QueueHandle handle)
{
	JobQueue* pJobQueue = GetJobQueue(handle);
	if (pJobQueue == nullptr) return;

	//Cancel all current JobQueue jobs (needed for integrity job)
	for (JobThread* pThread : m_WorkerThreads)
	{
		Job* pJob = pThread->GetRunningJob();
		if (pJob && pJob->m_JobQueueHandle == handle)
		{
			pJob->Canceled();
		}
	}

	//Take lock so we can remove the jobqueue from the list
	std::unique_lock<std::shared_timed_mutex> lock(m_JobMutex);
	m_JobQueues.erase(handle);

	//Again cancel any current JobQueue jobs (new ones could have been popped) and wait for them to finish
	for (JobThread* pThread : m_WorkerThreads)
	{
		Job* pJob = pThread->GetRunningJob();
		if (pJob && pJob->m_JobQueueHandle == handle)
		{
			pJob->Canceled();
			WaitForJob(pJob);
		}
	}

	//Delete remaining jobs and the JobQueue itself
	delete pJobQueue;

	//Final finish job check to make sure all jobs from this JobQueue are processed by the system
	ProcessFinishedJobs();
}

void JobManager::ShelveJobQueue(QueueHandle handle)
{
	JobQueue* pJobQueue = GetJobQueue(handle);
	if (pJobQueue == nullptr) return;

	//Cancel all current JobQueue jobs (needed for integrity job)
	for (JobThread* pThread : m_WorkerThreads)
	{
		Job* pJob = pThread->GetRunningJob();
		if (pJob && pJob->m_JobQueueHandle == handle)
		{
			pJob->Canceled();
		}
	}

	//Again cancel any current JobQueue jobs (new ones could have been popped) and wait for them to finish
	std::unique_lock<std::shared_timed_mutex> lock(m_JobMutex);
	m_JobQueues.erase(handle);

	//Again cancel any current JobQueue jobs (new ones could have been popped) and wait for them to finish
	for (JobThread* pThread : m_WorkerThreads)
	{
		Job* pJob = pThread->GetRunningJob();
		if (pJob && pJob->m_JobQueueHandle == handle)
		{
			pJob->Canceled();
			WaitForJob(pJob);
		}
	}
	m_ShelvedJobQueues[handle] = pJobQueue;

	ProcessFinishedJobs();
}

void JobManager::UnShelveJobQueue(QueueHandle handle)
{
	auto jobQueueIter = m_ShelvedJobQueues.find(handle);
	if (jobQueueIter == m_ShelvedJobQueues.end()) return;

	JobQueue* pJobQueue = jobQueueIter->second;

	std::unique_lock<std::shared_timed_mutex> lock(m_JobMutex);
	m_JobQueues[handle] = pJobQueue;
	m_ShelvedJobQueues.erase(handle);
}