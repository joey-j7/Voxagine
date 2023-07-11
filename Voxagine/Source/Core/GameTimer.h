#pragma once

#include "Core/Settings.h"
#include "Core/Time.h"

class GameTimer
{
public:
	GameTimer()
	{
		m_uiTargetElapsedTicks = GetTicksPerSecond() * 60;
		m_uiFrameLimitTicks = GetTicksPerSecond() * 200;
	}
	virtual ~GameTimer() {}

	// Get elapsed time since the previous Update call.
	uint64_t GetElapsedTicks() const { return m_uiElapsedTicks; }
	double GetElapsedSeconds() const { return TicksToSeconds(m_uiElapsedTicks); }

	// Get total time since the start of the program.
	uint64_t GetTotalTicks() const { return m_uiTotalTicks; }
	double GetTotalSeconds() const { return TicksToSeconds(m_uiTotalTicks); }

	// Get total number of updates since start of the program.
	uint32_t GetFrameCount() const { return m_uiFrameCount; }

	// Get the current framerate.
	uint32_t GetFramesPerSecond() const { return m_uiFramesPerSecond; }

	// Set whether to use fixed or variable timestep mode.
	void SetFixedTimeStep(bool bIsFixedTimestep) { m_bIsFixedTimeStep = bIsFixedTimestep; }

	// Set how often to call Update when in fixed timestep mode.
	void SetTargetElapsedTicks(uint64_t uiTargetElapsed) { m_uiTargetElapsedTicks = uiTargetElapsed; }
	void SetTargetElapsedSeconds(double dTargetElapsed) { m_uiTargetElapsedTicks = SecondsToTicks(dTargetElapsed); }

	void SetFrameLimitTicks(uint64_t uiFrameLimit) { m_uiFrameLimitTicks = uiFrameLimit; }
	void SetFrameLimitSeconds(double dFrameLimit) { m_uiFrameLimitTicks = SecondsToTicks(dFrameLimit); }

	virtual uint64_t GetTicksPerSecond() const { return 1000000000LL; };

	double TicksToSeconds(uint64_t uiTicks) const { return static_cast<double>(uiTicks) / GetTicksPerSecond(); }
	uint64_t SecondsToTicks(double dSeconds) const { return static_cast<uint64_t>(dSeconds * GetTicksPerSecond()); }

	// Getter for the current system time
	virtual const Time& GetCurrentSystemTime() const = 0;

	// After an intentional timing discontinuity (for instance a blocking IO operation)
	// call this to avoid having the fixed timestep logic attempt a set of catch-up 
	// Update calls.
	virtual void ResetElapsedTime()
	{
		m_uiLeftOverTicks = 0;
		m_uiFramesPerSecond = 0;
		m_uiFramesThisSecond = 0;
	}

	// Update timer state, calling the specified Update function the appropriate number of times.
	virtual void Update(const std::function<void()>& Update) = 0;

protected:
	// Derived timing data uses a canonical tick format.
	uint64_t m_uiElapsedTicks = 0;
	uint64_t m_uiTotalTicks = 0;
	uint64_t m_uiLeftOverTicks = 0;

	//Frame limit
	uint64_t m_uiFrameLimitTicks = 0;

	// Members for tracking the framerate.
	uint32_t m_uiFrameCount = 0;
	uint32_t m_uiFramesPerSecond = 0;
	uint32_t m_uiFramesThisSecond = 0;

	// Members for configuring fixed timestep mode.
	bool m_bIsFixedTimeStep = false;
	uint64_t m_uiTargetElapsedTicks = 0;

	// Member variable for representing system time
	mutable Time m_Time = Time{ 0, 0, 0 };
	mutable bool m_bIsTimeDirty = true;
};