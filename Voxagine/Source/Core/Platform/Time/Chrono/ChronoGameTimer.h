#pragma once

#include "Core/GameTimer.h"

#include <chrono>

/* Portable GameTimer built on steady_clock. Replaces the QueryPerformanceCounter
   based WINGameTimer; the tick accounting below is a straight port of it. */
class ChronoGameTimer : public GameTimer
{
public:
	typedef std::chrono::steady_clock Clock;

	ChronoGameTimer();
	virtual ~ChronoGameTimer() {}

	virtual uint64_t GetTicksPerSecond() const override;

	virtual const Time& GetCurrentSystemTime() const override;

	virtual void Update(const std::function<void()>& Update) override;

private:
	Clock::time_point m_LastTime;

	uint64_t m_uiSecondCounter = 0;
	uint64_t m_uiMaxDelta = 0;

	/* Sub-tick residue from converting clock units to canonical ticks, carried
	   into the next Update so no real time is lost to truncation. */
	uint64_t m_uiConversionRemainder = 0;
};
