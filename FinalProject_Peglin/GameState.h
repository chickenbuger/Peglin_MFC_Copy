#pragma once

enum class GameState
{
	Aiming,
	BallInFlight,
	ResolvingTurn,
	Victory,
	Defeat,
	Paused
};

constexpr bool CanTransitionTo(GameState from, GameState to) noexcept
{
	if (from == to)
	{
		return true;
	}

	switch (from)
	{
	case GameState::Aiming:
		return to == GameState::BallInFlight || to == GameState::Paused;
	case GameState::BallInFlight:
		return to == GameState::ResolvingTurn || to == GameState::Aiming || to == GameState::Paused;
	case GameState::ResolvingTurn:
		return to == GameState::Aiming || to == GameState::Victory || to == GameState::Defeat;
	case GameState::Victory:
	case GameState::Defeat:
		return to == GameState::Aiming;
	case GameState::Paused:
		return to == GameState::Aiming || to == GameState::BallInFlight;
	}

	return false;
}
