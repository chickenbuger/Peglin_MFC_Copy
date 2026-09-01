#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

struct PerformanceSnapshot
{
	std::size_t sampleCount = 0;
	float framesPerSecond = 0.0f;
	float averageFrameMilliseconds = 0.0f;
	float maximumFrameMilliseconds = 0.0f;
	int lastFixedSteps = 0;
	int peakFixedSteps = 0;
};

class PerformanceMonitor
{
public:
	static constexpr std::size_t SampleCapacity = 240;

	void RecordFrame(float elapsedSeconds, int fixedSteps) noexcept
	{
		if (elapsedSeconds <= 0.0f) return;
		const float milliseconds = (std::min)(elapsedSeconds, 1.0f) * 1000.0f;
		if (_sampleCount == SampleCapacity)
		{
			_sumMilliseconds -= _frameMilliseconds[_nextIndex];
		}
		else
		{
			++_sampleCount;
		}
		_frameMilliseconds[_nextIndex] = milliseconds;
		_sumMilliseconds += milliseconds;
		_nextIndex = (_nextIndex + 1U) % SampleCapacity;
		_lastFixedSteps = (std::max)(0, fixedSteps);
		_peakFixedSteps = (std::max)(_peakFixedSteps, _lastFixedSteps);
	}

	PerformanceSnapshot GetSnapshot() const noexcept
	{
		PerformanceSnapshot snapshot;
		snapshot.sampleCount = _sampleCount;
		snapshot.lastFixedSteps = _lastFixedSteps;
		snapshot.peakFixedSteps = _peakFixedSteps;
		if (_sampleCount == 0) return snapshot;
		snapshot.averageFrameMilliseconds = _sumMilliseconds / static_cast<float>(_sampleCount);
		snapshot.framesPerSecond = snapshot.averageFrameMilliseconds > 0.0f
			? 1000.0f / snapshot.averageFrameMilliseconds
			: 0.0f;
		for (std::size_t index = 0; index < _sampleCount; ++index)
		{
			snapshot.maximumFrameMilliseconds = (std::max)(
				snapshot.maximumFrameMilliseconds,
				_frameMilliseconds[index]);
		}
		return snapshot;
	}

private:
	std::array<float, SampleCapacity> _frameMilliseconds{};
	std::size_t _nextIndex = 0;
	std::size_t _sampleCount = 0;
	float _sumMilliseconds = 0.0f;
	int _lastFixedSteps = 0;
	int _peakFixedSteps = 0;
};
