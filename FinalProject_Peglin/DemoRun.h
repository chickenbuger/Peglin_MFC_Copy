#pragma once

#include "GameState.h"
#include "Vector2.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

enum class DemoActionType
{
	BeginAim,
	Fire
};

struct DemoAction
{
	DemoActionType type = DemoActionType::BeginAim;
	Vector2 direction{ 0.0f, -1.0f };
	float dragDistance = 160.0f;
	std::size_t shotIndex = 0;
};

inline bool HasDemoCommandLineFlag(std::wstring_view commandLine) noexcept
{
	constexpr std::wstring_view flag = L"--demo";
	std::size_t position = commandLine.find(flag);
	while (position != std::wstring_view::npos)
	{
		const bool validBefore = position == 0
			|| commandLine[position - 1] == L' '
			|| commandLine[position - 1] == L'\t'
			|| commandLine[position - 1] == L'"';
		const std::size_t after = position + flag.size();
		const bool validAfter = after == commandLine.size()
			|| commandLine[after] == L' '
			|| commandLine[after] == L'\t'
			|| commandLine[after] == L'"';
		if (validBefore && validAfter) return true;
		position = commandLine.find(flag, position + 1);
	}
	return false;
}

class DemoRunController
{
public:
	void SetEnabled(bool enabled) noexcept
	{
		_enabled = enabled;
		ResetSequence();
	}

	bool IsEnabled() const noexcept { return _enabled; }
	std::size_t GetNextShotIndex() const noexcept { return _nextShotIndex; }

	std::optional<DemoAction> Update(float deltaSeconds, GameState state) noexcept
	{
		if (!_enabled) return std::nullopt;
		if (state != GameState::Aiming)
		{
			_inAimingState = false;
			_aimIssued = false;
			_shotIssued = false;
			_stateSeconds = 0.0f;
			return std::nullopt;
		}
		if (!_inAimingState)
		{
			_inAimingState = true;
			_stateSeconds = 0.0f;
		}
		_stateSeconds += deltaSeconds > 0.0f ? deltaSeconds : 0.0f;

		const DemoShot& shot = SHOTS[_nextShotIndex % SHOTS.size()];
		if (!_aimIssued && _stateSeconds >= AIM_DELAY_SECONDS)
		{
			_aimIssued = true;
			return DemoAction{ DemoActionType::BeginAim, shot.direction, shot.dragDistance, _nextShotIndex };
		}
		if (_aimIssued && !_shotIssued && _stateSeconds >= FIRE_DELAY_SECONDS)
		{
			_shotIssued = true;
			const std::size_t firedIndex = _nextShotIndex;
			++_nextShotIndex;
			return DemoAction{ DemoActionType::Fire, shot.direction, shot.dragDistance, firedIndex };
		}
		return std::nullopt;
	}

private:
	struct DemoShot
	{
		Vector2 direction;
		float dragDistance;
	};

	void ResetSequence() noexcept
	{
		_nextShotIndex = 0;
		_stateSeconds = 0.0f;
		_inAimingState = false;
		_aimIssued = false;
		_shotIssued = false;
	}

	static constexpr float AIM_DELAY_SECONDS = 0.55f;
	static constexpr float FIRE_DELAY_SECONDS = 1.25f;
	static constexpr std::array<DemoShot, 6> SHOTS{
		DemoShot{ { 0.0f, -1.0f }, 170.0f },
		DemoShot{ { -0.28f, -0.96f }, 185.0f },
		DemoShot{ { 0.32f, -0.95f }, 180.0f },
		DemoShot{ { -0.52f, -0.85f }, 165.0f },
		DemoShot{ { 0.48f, -0.88f }, 190.0f },
		DemoShot{ { 0.12f, -0.99f }, 175.0f }
	};

	bool _enabled = false;
	std::size_t _nextShotIndex = 0;
	float _stateSeconds = 0.0f;
	bool _inAimingState = false;
	bool _aimIssued = false;
	bool _shotIssued = false;
};
