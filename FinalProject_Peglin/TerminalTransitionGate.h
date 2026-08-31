#pragma once

#include "GameWorld.h"

class TerminalTransitionGate
{
public:
	void Queue(GameUpdateResult result) noexcept
	{
		if (_pending == GameUpdateResult::None && result != GameUpdateResult::None)
		{
			_pending = result;
		}
	}

	bool IsPending() const noexcept
	{
		return _pending != GameUpdateResult::None;
	}

	GameUpdateResult CompleteIfReady(
		bool attackAnimationsFinished,
		bool feedbackAnimationsFinished) noexcept
	{
		if (!IsPending() || !attackAnimationsFinished || !feedbackAnimationsFinished)
		{
			return GameUpdateResult::None;
		}

		const GameUpdateResult completed = _pending;
		_pending = GameUpdateResult::None;
		return completed;
	}

	void Reset() noexcept
	{
		_pending = GameUpdateResult::None;
	}

private:
	GameUpdateResult _pending = GameUpdateResult::None;
};
