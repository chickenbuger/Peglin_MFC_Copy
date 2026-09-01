#include "pch.h"
#include "GamepadFeedback.h"

#include <algorithm>

namespace
{
	struct RumblePreset
	{
		float left;
		float right;
		float durationSeconds;
		int priority;
	};

	RumblePreset PresetFor(GamepadRumbleCue cue) noexcept
	{
		switch (cue)
		{
		case GamepadRumbleCue::PegHit: return { 0.08f, 0.24f, 0.055f, 1 };
		case GamepadRumbleCue::HeavyImpact: return { 0.65f, 0.90f, 0.13f, 2 };
		case GamepadRumbleCue::PlayerDamaged: return { 1.00f, 0.62f, 0.22f, 3 };
		case GamepadRumbleCue::Victory: return { 0.55f, 1.00f, 0.42f, 4 };
		case GamepadRumbleCue::Defeat: return { 1.00f, 0.45f, 0.48f, 4 };
		}
		return {};
	}
}

GamepadConnectionChange GamepadConnectionTracker::Update(bool connected) noexcept
{
	if (connected == _connected)
	{
		return GamepadConnectionChange::None;
	}
	_connected = connected;
	if (!connected)
	{
		return GamepadConnectionChange::Disconnected;
	}
	const bool reconnecting = _hasConnectedBefore;
	_hasConnectedBefore = true;
	return reconnecting
		? GamepadConnectionChange::Reconnected
		: GamepadConnectionChange::Connected;
}

void GamepadRumbleEnvelope::Trigger(GamepadRumbleCue cue) noexcept
{
	const RumblePreset preset = PresetFor(cue);
	if (IsActive() && preset.priority < _priority)
	{
		return;
	}
	_leftMotor = preset.left;
	_rightMotor = preset.right;
	_durationSeconds = preset.durationSeconds;
	_remainingSeconds = preset.durationSeconds;
	_priority = preset.priority;
}

void GamepadRumbleEnvelope::Update(float deltaSeconds) noexcept
{
	_remainingSeconds = (std::max)(0.0f, _remainingSeconds - (std::max)(0.0f, deltaSeconds));
	if (_remainingSeconds <= 0.0f)
	{
		Reset();
	}
}

void GamepadRumbleEnvelope::Reset() noexcept
{
	_leftMotor = 0.0f;
	_rightMotor = 0.0f;
	_durationSeconds = 0.0f;
	_remainingSeconds = 0.0f;
	_priority = 0;
}

GamepadMotorLevels GamepadRumbleEnvelope::GetLevels() const noexcept
{
	if (!IsActive() || _durationSeconds <= 0.0f)
	{
		return {};
	}
	constexpr float FADE_FRACTION = 0.35f;
	const float fadeSeconds = _durationSeconds * FADE_FRACTION;
	const float strength = _remainingSeconds >= fadeSeconds
		? 1.0f
		: std::clamp(_remainingSeconds / fadeSeconds, 0.0f, 1.0f);
	return { _leftMotor * strength, _rightMotor * strength };
}
