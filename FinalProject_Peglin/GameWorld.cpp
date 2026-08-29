#include "pch.h"
#include "GameWorld.h"

#include <cmath>

namespace
{
	constexpr int PEG_COLUMNS = 12;
	constexpr int PEG_ROWS = 4;
	constexpr float PEG_START_X = 50.0f;
	constexpr float PEG_START_Y = 400.0f;
	constexpr float PEG_SPACING = 80.0f;
	constexpr float PLAYFIELD_BOTTOM = 800.0f;
	constexpr float ENEMY_STEP = 64.0f;
	constexpr int ENEMY_STEPS_BEFORE_ATTACK = 8;
	constexpr float PLAYER_DAMAGE = 20.0f;
}

GameWorld::GameWorld()
{
	ResetGame();
}

GameUpdateResult GameWorld::Update(float deltaSeconds)
{
	switch (_gameState)
	{
	case GameState::Aiming:
	case GameState::Paused:
		break;
	case GameState::BallInFlight:
		HandlePegCollisions();
		_ball.update(deltaSeconds);
		if (_ball.GetPos()[1] > PLAYFIELD_BOTTOM)
		{
			TransitionTo(GameState::ResolvingTurn);
		}
		break;
	case GameState::ResolvingTurn:
		ResolveTurn();
		if (_gameState == GameState::Victory)
		{
			return GameUpdateResult::Victory;
		}
		if (_gameState == GameState::Defeat)
		{
			return GameUpdateResult::Defeat;
		}
		break;
	case GameState::Victory:
		return GameUpdateResult::Victory;
	case GameState::Defeat:
		return GameUpdateResult::Defeat;
	}

	return GameUpdateResult::None;
}

void GameWorld::ResetGame()
{
	_gameState = GameState::Aiming;
	_stateBeforePause = GameState::Aiming;
	_pendingDamage = 0.0f;
	_ball.Init();
	_player.Init();
	_enemy.Init();
	InitializeTargets();
}

void GameWorld::ResetBallToAiming()
{
	_ball.Init();
	_pendingDamage = 0.0f;
	TransitionTo(GameState::Aiming);
}

bool GameWorld::BeginAim(float x, float y)
{
	if (_gameState != GameState::Aiming || _ball.GetActive())
	{
		return false;
	}

	_ball.SetStartDragPos(x, y);
	_ball.SetTraceDragPos(x, y);
	_ball.SetClick(true);
	return true;
}

void GameWorld::UpdateAim(float x, float y)
{
	if (_gameState == GameState::Aiming && _ball.GetClick())
	{
		_ball.SetTraceDragPos(x, y);
	}
}

bool GameWorld::ReleaseShot(float x, float y)
{
	if (_gameState != GameState::Aiming || !_ball.GetClick())
	{
		return false;
	}

	bool launched = false;
	if (!_ball.GetActive())
	{
		_ball.SetEndDragPos(x, y);
		launched = _ball.shooting();
		if (launched)
		{
			TransitionTo(GameState::BallInFlight);
		}
	}

	_ball.SetClick(false);
	return launched;
}

void GameWorld::CancelAim()
{
	_ball.SetClick(false);
}

bool GameWorld::TogglePause()
{
	if (_gameState == GameState::Paused)
	{
		_ball.stop = false;
		return TransitionTo(_stateBeforePause);
	}

	if (_gameState == GameState::Aiming || _gameState == GameState::BallInFlight)
	{
		_stateBeforePause = _gameState;
		_ball.stop = true;
		return TransitionTo(GameState::Paused);
	}

	return false;
}

void GameWorld::InitializeTargets()
{
	_targetBallList._targetBallList.RemoveAll();
	for (int column = 0; column < PEG_COLUMNS; ++column)
	{
		for (int row = 0; row < PEG_ROWS; ++row)
		{
			TargetBall ball;
			ball.setting(
				PEG_START_X + static_cast<float>(column) * PEG_SPACING,
				PEG_START_Y + static_cast<float>(row) * PEG_SPACING);
			_targetBallList.add(ball);
		}
	}
}

void GameWorld::HandlePegCollisions()
{
	auto position = _targetBallList._targetBallList.GetHeadPosition();
	while (position != nullptr)
	{
		auto current = position;
		auto& target = _targetBallList._targetBallList.GetNext(position);
		const float ballX = _ball.GetPos()[0];
		const float ballY = _ball.GetPos()[1];
		const float offsetX = ballX - target.x;
		const float offsetY = ballY - target.y;
		const float distanceSquared = offsetX * offsetX + offsetY * offsetY;
		const float collisionRadius = _ball.GetSize() + target.size;

		if (distanceSquared <= collisionRadius * collisionRadius)
		{
			float distance = std::sqrt(distanceSquared);
			if (distance == 0.0f)
			{
				distance = 0.01f;
			}

			const float normalX = offsetX / distance;
			const float normalY = offsetY / distance;
			const float tangentX = -normalY;
			const float tangentY = normalX;
			const float tangentVelocity =
				_ball.GetVelocityX() * tangentX + _ball.GetVelocityY() * tangentY;

			_ball.SetVelocityX(tangentVelocity * tangentX);
			_ball.SetVelocityY(tangentVelocity * tangentY);
			_targetBallList._targetBallList.RemoveAt(current);
			_pendingDamage += 1.0f;
		}
	}
}

void GameWorld::ResolveTurn()
{
	if (_enemy.GetCount() < ENEMY_STEPS_BEFORE_ATTACK)
	{
		_enemy.SetX(_enemy.GetX() - ENEMY_STEP);
	}
	else
	{
		_player.SetHp(_player.GetHp() - PLAYER_DAMAGE);
	}

	_enemy.SetCount(_enemy.GetCount() + 1);
	_enemy.SetHp(_enemy.GetHp() - _pendingDamage);
	_pendingDamage = 0.0f;
	_ball.Init();

	if (_player.GetHp() <= 0.0f)
	{
		TransitionTo(GameState::Defeat);
	}
	else if (_enemy.GetHp() <= 0.0f)
	{
		TransitionTo(GameState::Victory);
	}
	else
	{
		TransitionTo(GameState::Aiming);
	}
}

bool GameWorld::TransitionTo(GameState nextState)
{
	if (!CanTransitionTo(_gameState, nextState))
	{
		return false;
	}

	_gameState = nextState;
	return true;
}
