#pragma once

#include "Enemy.h"
#include "GameState.h"
#include "Parent_ball.h"
#include "Player.h"
#include "TargetBall.h"

enum class GameUpdateResult
{
	None,
	Victory,
	Defeat
};

class GameWorld
{
public:
	GameWorld();

	GameUpdateResult Update(float deltaSeconds);
	void ResetGame();
	void ResetBallToAiming();

	bool BeginAim(float x, float y);
	void UpdateAim(float x, float y);
	bool ReleaseShot(float x, float y);
	void CancelAim();
	bool TogglePause();

	Player& GetPlayer() noexcept { return _player; }
	Enemy& GetEnemy() noexcept { return _enemy; }
	Parent_ball& GetBall() noexcept { return _ball; }
	TargetBallList& GetTargets() noexcept { return _targetBallList; }
	GameState GetState() const noexcept { return _gameState; }

private:
	void InitializeTargets();
	void HandlePegCollisions();
	void ResolveTurn();
	bool TransitionTo(GameState nextState);

	Player _player;
	Enemy _enemy;
	Parent_ball _ball;
	TargetBallList _targetBallList;
	float _pendingDamage = 0.0f;
	GameState _gameState = GameState::Aiming;
	GameState _stateBeforePause = GameState::Aiming;
};
