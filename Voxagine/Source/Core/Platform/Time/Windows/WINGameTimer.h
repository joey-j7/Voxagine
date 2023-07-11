#pragma once
#include "Core/GameTimer.h"

#include <cmath>
#include <exception>
#include <stdint.h>
#include <eventtoken.h>

#include <chrono>
#include <ctime>

class WINGameTimer : public GameTimer
{
public:
	WINGameTimer();

	const Time& GetCurrentSystemTime() const override;

	// Update timer state, calling the specified Update function the appropriate number of times.
	void Update(const std::function<void()>& Update) override;

	// Integer format represents time using 10,000,000 ticks per second.
	virtual uint64_t GetTicksPerSecond() const override;

private:
	LARGE_INTEGER m_iQpcLastTime;
	LARGE_INTEGER m_iQpcFrequency;
	uint64_t m_uiQpcMaxDelta = 0;
	uint64_t m_uiQpcSecondCounter = 0;
};