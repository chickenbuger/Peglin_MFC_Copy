#pragma once

#include "GameState.h"

enum class CombatPresentationPhase
{
	Aim,
	Pegboard,
	Attack,
	Enemy
};

constexpr CombatPresentationPhase ResolveCombatPresentationPhase(
	GameState state,
	bool playerAttackActive,
	bool enemyActionActive) noexcept
{
	if (enemyActionActive)
	{
		return CombatPresentationPhase::Enemy;
	}
	if (playerAttackActive || state == GameState::ResolvingTurn
		|| state == GameState::Victory || state == GameState::Defeat)
	{
		return CombatPresentationPhase::Attack;
	}
	if (state == GameState::BallInFlight)
	{
		return CombatPresentationPhase::Pegboard;
	}
	return CombatPresentationPhase::Aim;
}
