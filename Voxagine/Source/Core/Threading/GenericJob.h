#pragma once
#include "Core/Threading/Job.h"
#include <functional>

template<typename T>
class GenericJob : public Job
{
public:
	GenericJob(std::function<T()> backgroundFunc, std::function<void(T)> callback);

	virtual void Run() override;
	virtual void Finish() override;
	virtual void Canceled() override {}

private:
	std::function<T()> m_BackgroundTask;
	std::function<void(T)> m_Callback;
	T m_Result;
};

template<typename T>
GenericJob<T>::GenericJob(std::function<T()> backgroundFunc, std::function<void(T)> callback)
{
	m_BackgroundTask = backgroundFunc;
	m_Callback = callback;
}

template<typename T>
void GenericJob<T>::Run()
{
	m_Result = m_BackgroundTask();
}

template<typename T>
void GenericJob<T>::Finish()
{
	m_Callback(m_Result);
}