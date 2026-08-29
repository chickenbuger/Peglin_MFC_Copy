#include "pch.h"
#include "GameWorld.h"
#include "GameLayout.h"
#include "Physics.h"

#include <cmath>
#include <utility>

namespace
{
	constexpr float PLAYER_DAMAGE = 20.0f;
	constexpr float COLLISION_EPSILON = 0.001f;
}


GameWorld::GameWorld(PegLayoutDefinition pegLayout)
	: _pegLayout(std::move(pegLayout))
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
		if (_ball.GetPosition().y > GameLayout::BallExitY)
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
	_feedback = {};
	_ball.Init();
	_player.Init();
	_enemy.Init();
	InitializeTargets();
}

void GameWorld::ResetBallToAiming()
{
	_ball.Init();
	_pendingDamage = 0.0f;
	_feedback = {};
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
			_feedback.type = GameFeedbackType::ShotLaunched;
			_feedback.currentShotPegHits = 0;
			_feedback.lastEnemyDamage = 0.0f;
			_feedback.lastPlayerDamage = 0.0f;
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
		const bool transitioned = TransitionTo(_stateBeforePause);
		_feedback.type = _stateBeforePause == GameState::BallInFlight
			? GameFeedbackType::ShotLaunched
			: GameFeedbackType::Ready;
		return transitioned;
	}

	if (_gameState == GameState::Aiming || _gameState == GameState::BallInFlight)
	{
		_stateBeforePause = _gameState;
		_ball.stop = true;
		const bool transitioned = TransitionTo(GameState::Paused);
		_feedback.type = GameFeedbackType::Paused;
		return transitioned;
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
	for (const Vector2 position : _pegLayout.positions)
	{
		TargetBall ball;
		ball.setting(position);
		_targetBallList.add(ball);
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
			_feedback.type = GameFeedbackType::PegHit;
			++_feedback.currentShotPegHits;
		}
	}
}

void GameWorld::ResolveTurn()
{
	_feedback.lastEnemyDamage = _pendingDamage;
	_feedback.lastPlayerDamage = 0.0f;

	if (_enemy.GetCount() < GameLayout::EnemyStepsBeforeAttack)
	{
		_enemy.SetX(_enemy.GetX() - GameLayout::EnemyStep);
	}
	else
	{
		_player.SetHp(_player.GetHp() - PLAYER_DAMAGE);
		_feedback.lastPlayerDamage = PLAYER_DAMAGE;
	}

	_enemy.SetCount(_enemy.GetCount() + 1);
	_feedback.turnNumber = _enemy.GetCount();
	_enemy.SetHp(_enemy.GetHp() - _pendingDamage);
	_pendingDamage = 0.0f;
	_ball.Init();

	if (_player.GetHp() <= 0.0f)
	{
		TransitionTo(GameState::Defeat);
		_feedback.type = GameFeedbackType::Defeat;
	}
	else if (_enemy.GetHp() <= 0.0f)
	{
		TransitionTo(GameState::Victory);
		_feedback.type = GameFeedbackType::Victory;
	}
	else
	{
		TransitionTo(GameState::Aiming);
		_feedback.type = _feedback.lastPlayerDamage > 0.0f
			? GameFeedbackType::PlayerDamaged
			: GameFeedbackType::TurnResolved;
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
