#include "pch.h"
#include "GameWorld.h"
#include "Physics.h"

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
	constexpr float COLLISION_EPSILON = 0.001f;
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
		if (_ball.GetPosition().y > PLAYFIELD_BOTTOM)
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

bool GameWorld::BeginAim(Vector2 position)
{
	if (_gameState != GameState::Aiming || _ball.GetActive())
	{
		return false;
	}

	_ball.SetStartDragPos(position);
	_ball.SetTraceDragPos(position);
	_ball.SetClick(true);
	return true;
}

void GameWorld::UpdateAim(Vector2 position)
{
	if (_gameState == GameState::Aiming && _ball.GetClick())
	{
		_ball.SetTraceDragPos(position);
	}
}

bool GameWorld::ReleaseShot(Vector2 position)
{
	if (_gameState != GameState::Aiming || !_ball.GetClick())
	{
		return false;
	}

	bool launched = false;
	if (!_ball.GetActive())
	{
		_ball.SetEndDragPos(position);
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

void GameWorld::SetPegRestitution(float restitution) noexcept
{
	_pegRestitution = ClampRestitution(restitution);
}

void GameWorld::InitializeTargets()
{
	_targetBallList._targetBallList.RemoveAll();
	for (int column = 0; column < PEG_COLUMNS; ++column)
	{
		for (int row = 0; row < PEG_ROWS; ++row)
		{
			TargetBall ball;
			ball.setting({
				PEG_START_X + static_cast<float>(column) * PEG_SPACING,
				PEG_START_Y + static_cast<float>(row) * PEG_SPACING });
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
		const Vector2 offset = _ball.GetPosition() - target.position;
		const float distanceSquared = offset.LengthSquared();
		const float collisionRadius = _ball.GetSize() + target.size;

		if (distanceSquared <= collisionRadius * collisionRadius)
		{
			const float distance = std::sqrt(distanceSquared);
			Vector2 normal;
			if (distance <= COLLISION_EPSILON)
			{
				normal = (_ball.GetVelocity() * -1.0f).Normalized();
				if (normal.LengthSquared() == 0.0f)
				{
					normal = { 0.0f, -1.0f };
				}
			}
			else
			{
				normal = offset / distance;
			}

			const float penetration = collisionRadius - distance;
			_ball.SetPosition(
				_ball.GetPosition() + normal * (penetration + COLLISION_EPSILON));
			_ball.SetVelocity(
				ReflectVelocity(_ball.GetVelocity(), normal, _pegRestitution));
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
