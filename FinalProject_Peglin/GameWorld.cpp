#include "pch.h"
#include "GameWorld.h"
#include "GameLayout.h"
#include "Physics.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	constexpr float COLLISION_EPSILON = 0.001f;
	constexpr int SCORE_PER_COMBO_STEP = 100;
	constexpr float AIM_MIN_DRAG_DISTANCE = 0.001f;
	constexpr float AIM_MIN_FORCE = 1.0f;
	constexpr float AIM_MAX_FORCE = 10.0f;
	constexpr float AIM_MAX_DRAG_DISTANCE = 400.0f;
	constexpr float AIM_PREVIEW_TIMESTEP_SCALE = 4.0f;
	constexpr float AIM_PREVIEW_GRAVITY = 0.01f;

	float AimForceFromDragDistance(float distance)
	{
		const float clampedDistance = std::clamp(
			distance,
			AIM_MIN_FORCE,
			AIM_MAX_DRAG_DISTANCE);
		const float converted = clampedDistance
			/ (AIM_MAX_DRAG_DISTANCE / AIM_MAX_FORCE);
		return std::clamp(converted, AIM_MIN_FORCE, AIM_MAX_FORCE);
	}

	StageDefinition LoadDefaultStage()
	{
		StageLoadResult result = LoadStageDefinition("stage-1");
		return result.IsSuccess()
			? std::move(*result.stage)
			: CreateDefaultStageDefinition();
	}

	StageDefinition CreateCustomStage(PegLayoutDefinition pegLayout)
	{
		StageDefinition stage = LoadDefaultStage();
		stage.id = "custom";
		stage.displayName = "Custom Stage";
		stage.pegLayout = std::move(pegLayout);
		return stage;
	}
}

GameWorld::GameWorld()
	: GameWorld(LoadDefaultStage())
{
}

GameWorld::GameWorld(PegLayoutDefinition pegLayout)
	: GameWorld(CreateCustomStage(std::move(pegLayout)))
{
}

GameWorld::GameWorld(StageDefinition stage)
	: _stage(std::move(stage)),
	_pegRestitution(_stage.rules.pegRestitution)
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
	_events.clear();
	_aimStart = _ball.GetPosition();
	_aimCurrent = _aimStart;
	_aimInProgress = false;
	_terminalResultReported = false;
	_pegRestitution = _stage.rules.pegRestitution;
	_ball.Init();
	_player.Init();
	_player.SetHp(_stage.rules.playerHealth);
	_enemy.Init();
	_enemy.SetHp(_stage.rules.enemyHealth);
	InitializeTargets();
}

void GameWorld::ResetBallToAiming()
{
	_ball.Init();
	_pendingDamage = 0.0f;
	_feedback = {};
	_score.currentShot = 0;
	_score.currentCombo = 0;
	_events.clear();
	_aimStart = _ball.GetPosition();
	_aimCurrent = _aimStart;
	_aimInProgress = false;
	_terminalResultReported = false;
	TransitionTo(GameState::Aiming);
}

bool GameWorld::LoadStage(std::string_view stageId, GameDifficulty difficulty)
{
	StageLoadResult result = LoadStageDefinition(stageId);
	if (!result.IsSuccess())
	{
		return false;
	}

	StageDefinition configuredStage = ApplyDifficulty(*result.stage, difficulty);
	if (!ValidateStageDefinition(configuredStage).IsValid())
	{
		return false;
	}

	_stage = std::move(configuredStage);
	_difficulty = difficulty;
	ResetGame();
	return true;
}

void GameWorld::ResetProgression()
{
	_loadout.Reset();
	ResetGame();
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
	_aimStart = position;
	_aimCurrent = position;
	_aimInProgress = true;
	return true;
}

void GameWorld::UpdateAim(Vector2 position)
{
	if (_gameState == GameState::Aiming && _ball.GetClick())
	{
		_ball.SetTraceDragPos(position);
		_aimCurrent = position;
	}
}

AimPreview GameWorld::GetAimPreview() const noexcept
{
	AimPreview preview;
	const Vector2 launchVector = _aimStart - _aimCurrent;
	preview.dragDistance = launchVector.Length();
	if (!_aimInProgress
		|| _gameState != GameState::Aiming
		|| preview.dragDistance <= AIM_MIN_DRAG_DISTANCE)
	{
		return preview;
	}

	preview.visible = true;
	preview.launchDirection = launchVector.Normalized();
	const float force = AimForceFromDragDistance(preview.dragDistance);
	preview.normalizedStrength = (force - AIM_MIN_FORCE)
		/ (AIM_MAX_FORCE - AIM_MIN_FORCE);

	Vector2 position = _ball.GetPosition();
	Vector2 velocity = preview.launchDirection;
	for (Vector2& point : preview.points)
	{
		velocity.y += AIM_PREVIEW_GRAVITY * AIM_PREVIEW_TIMESTEP_SCALE;
		position += velocity * (force * AIM_PREVIEW_TIMESTEP_SCALE);

		if (position.x < GameLayout::BallLeftBoundary)
		{
			position.x = GameLayout::BallLeftBoundary;
			velocity.x = std::fabs(velocity.x);
		}
		else if (position.x > GameLayout::BallRightBoundary)
		{
			position.x = GameLayout::BallRightBoundary;
			velocity.x = -std::fabs(velocity.x);
		}
		if (position.y < GameLayout::BallTopBoundary)
		{
			position.y = GameLayout::BallTopBoundary;
			velocity.y = std::fabs(velocity.y);
		}

		point = position;
	}

	return preview;
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
	_aimInProgress = false;
	return launched;
}

void GameWorld::CancelAim()
{
	_ball.SetClick(false);
	_aimInProgress = false;
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
	for (const PegDefinition& definition : _stage.pegLayout.pegs)
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
	const ProgressionModifiers modifiers = _loadout.CalculateModifiers();
	const float awardedDamage = definition.damage * modifiers.pegDamageMultiplier;
	_pendingDamage += awardedDamage;
	++_score.currentCombo;
	_score.bestCombo = (std::max)(_score.bestCombo, _score.currentCombo);
	const float rawScore = static_cast<float>(SCORE_PER_COMBO_STEP
		* _score.currentCombo
		* definition.scoreMultiplier)
		* modifiers.scoreMultiplier;
	const int scoreAwarded = static_cast<int>(std::lround(rawScore));
	_score.currentShot += scoreAwarded;
	_feedback.type = GameFeedbackType::PegHit;
	++_feedback.currentShotPegHits;
	_events.push_back({
		GameEventType::PegHit,
		target.position,
		target.type,
		scoreAwarded,
		_score.currentCombo,
		1,
		awardedDamage
	});
}

void GameWorld::ApplyBombEffect(const TargetBall& bomb)
{
	const float blastRadius = GetPegTypeDefinition(bomb.type).blastRadius;
	const float blastRadiusSquared = blastRadius * blastRadius;
	int removedPegs = 0;
	auto position = _targetBallList._targetBallList.GetHeadPosition();
	while (position != nullptr)
	{
		auto current = position;
		const TargetBall target = _targetBallList._targetBallList.GetNext(position);
		if ((target.position - bomb.position).LengthSquared() <= blastRadiusSquared)
		{
			_targetBallList._targetBallList.RemoveAt(current);
			AwardPeg(target);
			++removedPegs;
		}
	}

	_events.push_back({
		GameEventType::BombTriggered,
		bomb.position,
		bomb.type,
		0,
		_score.currentCombo,
		removedPegs,
		0.0f
	});
}

void GameWorld::RestoreRemovedPegs(Vector2 excludedPosition)
{
	constexpr float POSITION_EPSILON_SQUARED = 0.0001f;
	int restoredPegs = 0;
	for (const PegDefinition& definition : _stage.pegLayout.pegs)
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
			++restoredPegs;
		}
	}

	_events.push_back({
		GameEventType::RefreshTriggered,
		excludedPosition,
		PegType::Refresh,
		0,
		_score.currentCombo,
		restoredPegs,
		0.0f
	});
}

void GameWorld::ResolveTurn()
{
	_feedback.lastEnemyDamage = _pendingDamage;
	_feedback.lastPlayerDamage = 0.0f;
	_score.lastTurn = _score.currentShot;
	_score.total += _score.lastTurn;
	_score.currentShot = 0;
	_score.currentCombo = 0;

	if (_enemy.GetCount() < _stage.rules.enemyStepsBeforeAttack)
	{
		_enemy.SetX(_enemy.GetX() - _stage.rules.enemyStep);
	}
	else
	{
		const ProgressionModifiers modifiers = _loadout.CalculateModifiers();
		const float incomingDamage = _stage.rules.playerDamage
			* modifiers.incomingDamageMultiplier;
		_player.SetHp(_player.GetHp() - incomingDamage);
		_feedback.lastPlayerDamage = incomingDamage;
	}

	_enemy.SetCount(_enemy.GetCount() + 1);
	_feedback.turnNumber = _enemy.GetCount();
	_enemy.SetHp(_enemy.GetHp() - _pendingDamage);
	_events.push_back({
		GameEventType::TurnResolved,
		{},
		PegType::Normal,
		_score.lastTurn,
		_score.bestCombo,
		_feedback.currentShotPegHits,
		_feedback.lastEnemyDamage
	});
	if (_feedback.lastPlayerDamage > 0.0f)
	{
		_events.push_back({
			GameEventType::PlayerDamaged,
			GameLayout::PlayerPosition,
			PegType::Normal,
			0,
			0,
			0,
			_feedback.lastPlayerDamage
		});
	}
	_pendingDamage = 0.0f;
	_ball.Init();

	if (_player.GetHp() <= 0.0f)
	{
		TransitionTo(GameState::Defeat);
		_feedback.type = GameFeedbackType::Defeat;
		_events.push_back({
			GameEventType::Defeat,
			{},
			PegType::Normal,
			0,
			0,
			0,
			0.0f
		});
	}
	else if (_enemy.GetHp() <= 0.0f)
	{
		TransitionTo(GameState::Victory);
		_feedback.type = GameFeedbackType::Victory;
		_events.push_back({
			GameEventType::Victory,
			{},
			PegType::Normal,
			0,
			0,
			0,
			0.0f
		});
	}
	else
	{
		TransitionTo(GameState::Aiming);
		_feedback.type = _feedback.lastPlayerDamage > 0.0f
			? GameFeedbackType::PlayerDamaged
			: GameFeedbackType::TurnResolved;
	}
}

std::vector<GameEvent> GameWorld::ConsumeEvents()
{
	std::vector<GameEvent> events;
	events.swap(_events);
	return events;
}

std::optional<GameResultSummary> GameWorld::GetResultSummary() const
{
	GameUpdateResult result = GameUpdateResult::None;
	if (_gameState == GameState::Victory)
	{
		result = GameUpdateResult::Victory;
	}
	else if (_gameState == GameState::Defeat)
	{
		result = GameUpdateResult::Defeat;
	}
	else
	{
		return std::nullopt;
	}

	return GameResultSummary{
		result,
		_stage.id,
		_stage.displayName,
		_score.total,
		_score.bestCombo,
		_enemy.GetCount()
	};
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
