#include "FrameProfiler.h"

#include <cmath>
#include <cstdio>

FrameProfiler& FrameProfiler::Get()
{
	static FrameProfiler instance;
	return instance;
}

void FrameProfiler::Report(const std::string& name, double fMilliseconds)
{
	Accumulator& accum = m_Accumulators[name];
	accum.fTotalMs += fMilliseconds;
	++accum.uiSamples;
}

void FrameProfiler::Tick(float fDeltaTime)
{
	if (!m_bEnabled)
		return;

	m_fTimer += fDeltaTime;

	if (m_fTimer < 1.0f)
		return;

	m_fTimer = std::fmod(m_fTimer, 1.0f);

	for (auto& [name, accum] : m_Accumulators)
	{
		if (accum.uiSamples == 0)
			continue;

		fprintf(stderr, "[timing] %-28s %6.3f ms (x%u/s)\n",
		        name.c_str(), accum.fTotalMs / accum.uiSamples, accum.uiSamples);

		accum.fTotalMs = 0.0;
		accum.uiSamples = 0;
	}
}
