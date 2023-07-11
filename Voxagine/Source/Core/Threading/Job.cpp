#include "pch.h"
#include "Core/Threading/Job.h"
#include "Core/Threading/JobManager.h"

void Job::Wait()
{
	if (m_pJobManager)
	{
		m_pJobManager->WaitForJob(this);
	}
}
