#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

class AudioEffectGate
{
public:
	void Update(float deltaSeconds) noexcept
	{
		_clockSeconds += (std::max)(0.0f, deltaSeconds);
		if (_clockSeconds - _windowStartSeconds >= BURST_WINDOW_SECONDS)
		{
			_windowStartSeconds = _clockSeconds;
			acceptedInWindow = 0;
		}
	}

	bool TryAccept(std::string_view cueId) noexcept
	{
		const bool priority = IsPriorityCue(cueId);
		const float sameCueCooldown = cueId == "peg_hit" ? 0.055f : 0.035f;
		if (!priority && cueId == _lastCue
			&& _clockSeconds - _lastCueSeconds < sameCueCooldown)
		{
			return false;
		}
		if (!priority && acceptedInWindow >= MAX_ORDINARY_EFFECTS_PER_WINDOW)
		{
			return false;
		}

		_lastCue.assign(cueId);
		_lastCueSeconds = _clockSeconds;
		if (!priority)
		{
			++acceptedInWindow;
		}
		return true;
	}

	int GetAcceptedInWindow() const noexcept { return acceptedInWindow; }

private:
	static bool IsPriorityCue(std::string_view cueId) noexcept
	{
		return cueId == "bomb" || cueId == "refresh" || cueId == "damage"
			|| cueId == "victory" || cueId == "defeat";
	}

	static constexpr float BURST_WINDOW_SECONDS = 0.1f;
	static constexpr int MAX_ORDINARY_EFFECTS_PER_WINDOW = 3;
	float _clockSeconds = 0.0f;
	float _windowStartSeconds = 0.0f;
	float _lastCueSeconds = -1.0f;
	int acceptedInWindow = 0;
	std::string _lastCue;
};

class AudioFadeEnvelope
{
public:
	void Reset(float value) noexcept
	{
		_current = std::clamp(value, 0.0f, 1.0f);
		_target = _current;
	}

	void SetTarget(float target) noexcept
	{
		_target = std::clamp(target, 0.0f, 1.0f);
	}

	float Update(float deltaSeconds, float fadeSeconds) noexcept
	{
		if (fadeSeconds <= 0.0f)
		{
			_current = _target;
			return _current;
		}
		const float step = (std::max)(0.0f, deltaSeconds) / fadeSeconds;
		if (_current < _target)
		{
			_current = (std::min)(_target, _current + step);
		}
		else
		{
			_current = (std::max)(_target, _current - step);
		}
		return _current;
	}

	float GetCurrent() const noexcept { return _current; }
	bool IsAtTarget(float tolerance = 0.001f) const noexcept
	{
		return std::fabs(_current - _target) <= tolerance;
	}

private:
	float _current = 1.0f;
	float _target = 1.0f;
};
