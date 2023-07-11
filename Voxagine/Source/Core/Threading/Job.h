#pragma once

enum JobType
{
	JT_DEFAULT,
	JT_PHYSICS,
	JT_IO
};

class JobManager;
class Job
{
public:
	friend class JobManager;
	friend class JobQueue;
	virtual ~Job() { }

	// Run is called asynchronously on a background thread 
	virtual void Run() = 0;

	// Finish is called on the main thread from which the job was enqueued
	virtual void Finish() = 0;

	// Called when the job has been canceled or has to stop running
	// Make sure any running operation is stopped when this function is called to prevent hanging of the application
	virtual void Canceled() = 0;

	// Blocks the calling thread till the job is finished (should only be called on the main thread)
	void Wait();

	// Returns true if the job is waiting to be executed
	bool IsWaiting() { return m_bJobWaiting; }

private:
	void SetJobManager(JobManager* pJobManager) { m_pJobManager = pJobManager; }
	void SetWaiting(bool bWaiting) { m_bJobWaiting = bWaiting; }
	void SetQueueHandle(uint32_t handle) { m_JobQueueHandle = handle; }

	JobManager* m_pJobManager = nullptr;
	JobType m_Type = JT_DEFAULT;
	uint32_t m_JobQueueHandle = UINT_MAX;
	bool m_bJobWaiting = true;
};