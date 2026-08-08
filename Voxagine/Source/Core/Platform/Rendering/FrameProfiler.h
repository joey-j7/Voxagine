#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

/* RENDERING_PLAN.md Phase 0: aggregates named GPU-pass and CPU-section
 * timings and logs a rolling average once per second, mirroring
 * RenderContext's existing [fps] log.
 *
 * Disabled by default outside _DEBUG (Settings::IsGPUProfilingEnabled) so a
 * Release build pays nothing beyond the IsEnabled() branch at each call
 * site - callers are expected to skip their own measurement work (a
 * std::chrono span, a GPU query) when this is false rather than call
 * Report() unconditionally. */
class FrameProfiler
{
public:
	static FrameProfiler& Get();

	void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }
	bool IsEnabled() const { return m_bEnabled; }

	void Report(const std::string& name, double fMilliseconds);

	/* Call once per frame; logs and resets every accumulator roughly once a
	   second. No-op when disabled. */
	void Tick(float fDeltaTime);

private:
	struct Accumulator
	{
		double fTotalMs = 0.0;

		/* The one-off costs are the interesting ones - a bake or a chunk load
		   happens on a single frame of a second and the average over 200 of
		   them buries it. Reported alongside the average, reset with it. */
		double fPeakMs = 0.0;

		uint32_t uiSamples = 0;
	};

	bool m_bEnabled = false;
	float m_fTimer = 0.0f;

	std::unordered_map<std::string, Accumulator> m_Accumulators;
};
