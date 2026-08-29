#include "pch.h"
#include "GameWorld.h"
#include "GameLayout.h"
#include "Physics.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	constexpr float PLAYER_DAMAGE = 20.0f;
	constexpr float COLLISION_EPSILON = 0.001f;
	constexpr int SCORE_PER_COMBO_STEP = 100;
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
			return ReportTerminalResult(GameUpdateResult::Victory);
		}
		if (_gameState == GameState::Defeat)
		{
			return ReportTerminalResult(GameUpdateResult::Defeat);
		}
		break;
	case GameState::Victory:
		return ReportTerminalResult(GameUpdateResult::Victory);
	case GameState::Defeat:
		return ReportTerminalResult(GameUpdateResult::Defeat);
	}

	return GameUpdateResult::None;
}

void GameWorld::ResetGame()
{
	_gameState = GameState::Aiming;
	_stateBeforePause = GameState::Aiming;
	_pendingDamage = 0.0f;
	_feedback = {};
	_score = {};
	_terminalResultReported = false;
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
	_score.currentShot = 0;
	_score.currentCombo = 0;
	_terminalResultReported = false;
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
			_score.currentShot = 0;
			_score.currentCombo = 0;
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
	for (const PegDefinition& definition : _pegLayout.pegs)
	{
		TargetBall ball;
		ball.setting(definition);
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
			const TargetBall hitTarget = target;
			_targetBallList._targetBallList.RemoveAt(current);
			AwardPeg(hitTarget);

			const PegTypeDefinition& definition = GetPegTypeDefinition(hitTarget.type);
			if (definition.blastRadius > 0.0f)
			{
				ApplyBombEffect(hitTarget);
			}
			if (definition.refreshesRemovedPegs)
			{
				RestoreRemovedPegs(hitTarget.position);
			}

			return;
		}
	}
}

void GameWorld::AwardPeg(const TargetBall& target)
{
	const PegTypeDefinition& definition = GetPegTypeDefinition(target.type);
	_pendingDamage += definition.damage;
	++_score.currentCombo;
	_score.bestCombo = (std::max)(_score.bestCombo, _score.currentCombo);
	_score.currentShot += SCORE_PER_COMBO_STEP
		* _score.currentCombo
		* definition.scoreMultiplier;
	_feedback.type = GameFeedbackType::PegHit;
	++_feedback.currentShotPegHits;
}

void GameWorld::ApplyBombEffect(const TargetBall& bomb)
{
	const float blastRadius = GetPegTypeDefinition(bomb.type).blastRadius;
	const float blastRadiusSquared = blastRadius * blastRadius;
	auto position = _targetBallList._targetBallList.GetHeadPosition();
	while (position != nullptr)
	{
		auto current = position;
		const TargetBall target = _targetBallList._targetBallList.GetNext(position);
		if ((target.position - bomb.position).LengthSquared() <= blastRadiusSquared)
		{
			_targetBallList._targetBallList.RemoveAt(current);
			AwardPeg(target);
		}
	}
}

void GameWorld::RestoreRemovedPegs(Vector2 excludedPosition)
{
	constexpr float POSITION_EPSILON_SQUARED = 0.0001f;
	for (const PegDefinition& definition : _pegLayout.pegs)
	{
		if ((definition.position - excludedPosition).LengthSquared() <= POSITION_EPSILON_SQUARED)
		{
			continue;
		}

		bool isActive = false;
		auto activePosition = _targetBallList._targetBallList.GetHeadPosition();
		while (activePosition != nullptr)
		{
			const TargetBall& active = _targetBallList._targetBallList.GetNext(activePosition);
			if ((active.position - definition.position).LengthSquared() <= POSITION_EPSILON_SQUARED)
			{
				isActive = true;
				break;
			}
		}

		if (!isActive)
		{
			TargetBall restored;
			restored.setting(definition);
			_targetBallList.add(restored);
		}
	}
}

void GameWorld::ResolveTurn()
{
	_feedback.lastEnemyDamage = _pendingDamage;
	_feedback.lastPlayerDamage = 0.0f;
	_score.lastTurn = _score.currentShot;
	_score.total += _score.lastTurn;
	_score.currentShot = 0;
	_score.currentCombo = 0;

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

GameUpdateResult GameWorld::ReportTerminalResult(GameUpdateResult result) noexcept
{
	if (_terminalResultReported)
	{
		return GameUpdateResult::None;
	}

	_terminalResultReported = true;
	return result;
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
