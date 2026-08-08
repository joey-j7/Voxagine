#include "pch.h"
#include "ChronoGameTimer.h"

#include <cmath>
#include <ctime>

/* steady_clock is counted in its own native period; everything below works in
   those raw units and only converts to canonical ticks once, the same way
   WINGameTimer worked in QPC units. */
static const uint64_t s_uiClockFrequency =
	static_cast<uint64_t>(ChronoGameTimer::Clock::period::den) /
	static_cast<uint64_t>(ChronoGameTimer::Clock::period::num);

ChronoGameTimer::ChronoGameTimer()
{
	m_LastTime = Clock::now();

	// Initialize max delta to 1/10 of a second.
	m_uiMaxDelta = s_uiClockFrequency / 10;
}

uint64_t ChronoGameTimer::GetTicksPerSecond() const
{
	return 10000000;
}

const Time& ChronoGameTimer::GetCurrentSystemTime() const
{
	//If dirty flag is true, then update the local time variable
	if (m_bIsTimeDirty)
	{
		std::tm bt{};
		std::time_t rawtime = std::time(nullptr);

		/* localtime_r is POSIX; MSVC offers localtime_s with the arguments the
		   other way round. Both are the thread-safe form. */
#ifdef _WIN32
		localtime_s(&bt, &rawtime);
#else
		localtime_r(&rawtime, &bt);
#endif

		m_Time.Hours = static_cast<char>(bt.tm_hour);
		m_Time.Minutes = static_cast<char>(bt.tm_min);
		m_Time.Seconds = static_cast<char>(bt.tm_sec);

		m_bIsTimeDirty = false;
	}

	// Return the current system time
	return m_Time;
}

void ChronoGameTimer::Update(const std::function<void()>& Update)
{
	//Set system time dirty value to true each update cycle
	m_bIsTimeDirty = true;

	const Clock::time_point CurrentTime = Clock::now();

	uint64_t TimeDelta = static_cast<uint64_t>((CurrentTime - m_LastTime).count());

	m_LastTime = CurrentTime;
	m_uiSecondCounter += TimeDelta;

	// Clamp excessively large time deltas (e.g. after paused in the debugger).
	if (TimeDelta > m_uiMaxDelta)
	{
		TimeDelta = m_uiMaxDelta;
	}

	// Convert clock units into a canonical tick format. This cannot overflow due to the previous clamp.
	// The division truncates, and m_LastTime has already advanced past the
	// truncated part - without carrying the remainder, a call rate faster than
	// one canonical tick (100ns) throws away nearly all real time and the main
	// loop's frame limiter only fires on the rare slow iteration.
	TimeDelta *= GetTicksPerSecond();
	TimeDelta += m_uiConversionRemainder;
	m_uiConversionRemainder = TimeDelta % s_uiClockFrequency;
	TimeDelta /= s_uiClockFrequency;

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

	if (m_uiSecondCounter >= s_uiClockFrequency)
	{
		m_uiFramesPerSecond = m_uiFramesThisSecond;
		m_uiFramesThisSecond = 0;
		m_uiSecondCounter %= s_uiClockFrequency;
	}
}
