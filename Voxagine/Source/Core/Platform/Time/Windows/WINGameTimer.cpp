#include "pch.h"
#include "WINGameTimer.h"

WINGameTimer::WINGameTimer()
{
	if (!QueryPerformanceFrequency(&m_iQpcFrequency))
	{
		throw std::exception("QueryPerformanceFrequency");
	}
		
	if (!QueryPerformanceCounter(&m_iQpcLastTime))
	{
		throw std::exception("QueryPerformanceCounter");
	}
		
	// Initialize max delta to 1/10 of a second.
	m_uiQpcMaxDelta = static_cast<uint64_t>(m_iQpcFrequency.QuadPart / 10);
}

uint64_t WINGameTimer::GetTicksPerSecond() const
{
	return 10000000;
}

const Time& WINGameTimer::GetCurrentSystemTime() const
{
	//If dirty flag is true, then update the local time variable
	if (m_bIsTimeDirty)
	{
		std::tm bt{};
		std::time_t rawtime = std::time(0);

		localtime_s(&bt, &rawtime);

		m_Time.Hours = static_cast<char>(bt.tm_hour);
		m_Time.Minutes = static_cast<char>(bt.tm_min);
		m_Time.Seconds = static_cast<char>(bt.tm_sec);

		m_bIsTimeDirty = false;
	}

	// Return the current system time 
	return m_Time;
}

void WINGameTimer::Update(const std::function<void()>& Update)
{
	 //Set system time dirty value to true each update cycle
	m_bIsTimeDirty = true;

	// Query the current time.
	LARGE_INTEGER CurrentTime;

	if (!QueryPerformanceCounter(&CurrentTime))
	{
		throw std::exception("QueryPerformanceCounter");
	}

	uint64_t TimeDelta = CurrentTime.QuadPart - m_iQpcLastTime.QuadPart;

	m_iQpcLastTime = CurrentTime;
	m_uiQpcSecondCounter += TimeDelta;

	// Clamp excessively large time deltas (e.g. after paused in the debugger).
	if (TimeDelta > m_uiQpcMaxDelta)
	{
		TimeDelta = m_uiQpcMaxDelta;
	}

	// Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
	TimeDelta *= GetTicksPerSecond();
	TimeDelta /= m_iQpcFrequency.QuadPart;

	uint32_t uiLastFrameCount = m_uiFrameCount;

	if (m_bIsFixedTimeStep)
	{
		// Fixed timestep update logic

		// If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
		// the clock to exactly match the target value. This prevents tiny and irrelevant errors
		// from accumulating over time. Without this clamping, a game that requested a 60 fps
		// fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
		// accumulate enough tiny errors that it would drop a frame. It is better to just round 
		// small deviations down to zero to leave things running smoothly.

		if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(TimeDelta - m_uiTargetElapsedTicks))) < GetTicksPerSecond() / 4000)
		{
			TimeDelta = m_uiTargetElapsedTicks;
		}

		m_uiLeftOverTicks += TimeDelta;

		bool isUpdated = false;
		while (m_uiLeftOverTicks >= m_uiTargetElapsedTicks)
		{
			isUpdated = true;
			m_uiElapsedTicks = m_uiTargetElapsedTicks;
			m_uiTotalTicks += m_uiTargetElapsedTicks;
			m_uiLeftOverTicks -= m_uiTargetElapsedTicks;
			m_uiFrameCount++;
		}

		if (isUpdated)
		{
			Update();
		}
	}
	else
	{
		m_uiLeftOverTicks += TimeDelta;
		if (m_uiLeftOverTicks >= m_uiFrameLimitTicks)
		{
			// Variable timestep update logic.
			m_uiElapsedTicks = m_uiLeftOverTicks;
			m_uiTotalTicks += m_uiLeftOverTicks;
			m_uiLeftOverTicks = 0;
			m_uiFrameCount++;

			Update();
		}
	}

	// Track the current framerate.
	if (m_uiFrameCount != uiLastFrameCount)
	{
		m_uiFramesThisSecond++;
	}

	if (m_uiQpcSecondCounter >= static_cast<uint64_t>(m_iQpcFrequency.QuadPart))
	{
		m_uiFramesPerSecond = m_uiFramesThisSecond;
		m_uiFramesThisSecond = 0;
		m_uiQpcSecondCounter %= m_iQpcFrequency.QuadPart;
	}
}
