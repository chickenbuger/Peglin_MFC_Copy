#pragma once

#include <algorithm>

class UiAnimationTimeline
{
public:
	void Start(float durationSeconds) noexcept
	{
		_durationSeconds = (std::max)(durationSeconds, 0.001f);
		_ageSeconds = 0.0f;
		_active = true;
	}

	void Update(float deltaSeconds) noexcept
	{
		if (!_active || deltaSeconds <= 0.0f)
		{
			return;
		}
		_ageSeconds = (std::min)(_durationSeconds, _ageSeconds + deltaSeconds);
		_active = _ageSeconds < _durationSeconds;
	}

	void Reset() noexcept
	{
		_ageSeconds = 0.0f;
		_durationSeconds = 1.0f;
		_active = false;
	}

	bool IsActive() const noexcept { return _active; }
	float Progress() const noexcept
	{
		return std::clamp(_ageSeconds / _durationSeconds, 0.0f, 1.0f);
	}
	float EaseOutProgress() const noexcept
	{
		const float remaining = 1.0f - Progress();
		return 1.0f - remaining * remaining;
	}

private:
	float _ageSeconds = 0.0f;
	float _durationSeconds = 1.0f;
	bool _active = false;
};
