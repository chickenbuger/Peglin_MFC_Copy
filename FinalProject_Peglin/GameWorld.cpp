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
	constexpr float AIM_PREVIEW_GRAVITY = 0.01f;
	constexpr float AIM_PREVIEW_STEP_LENGTH =
		AimPreview::GuideLengthPixels / static_cast<float>(AimPreview::PointCount);

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
	_loadout.BeginBattle(GetBattleShuffleSeed());
	_gameState = GameState::Aiming;
	_stateBeforePause = GameState::Aiming;
	_pendingDamage = 0.0f;
	_completedTurns = 0;
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
	_enemies.clear();
	std::vector<EnemyDefinition> enemyDefinitions = _stage.enemies;
	if (enemyDefinitions.empty())
	{
		enemyDefinitions.push_back({
			"legacy-enemy",
			_stage.isBoss ? "Rootbound Titan" : "Crystal Toad",
			EnemyVisualKind::CrystalToad,
			_stage.rules.enemyHealth
		});
	}
	const bool isGroup = enemyDefinitions.size() > 1;
	for (std::size_t index = 0; index < enemyDefinitions.size(); ++index)
	{
		EnemyCombatant combatant;
		combatant.definition = enemyDefinitions[index];
		combatant.actor.Init();
		combatant.actor.SetHp(combatant.definition.health);
		combatant.actor.SetX(
			isGroup
				? GameLayout::EnemyGroupStartX + GameLayout::EnemyGroupSpacing * static_cast<float>(index)
				: GameLayout::EnemyInitialPosition.x);
		_enemies.push_back(std::move(combatant));
	}
	_activeEnemyIndex = 0;
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
	return LoadStage(*result.stage, difficulty);
}

bool GameWorld::LoadStage(const StageDefinition& stage, GameDifficulty difficulty)
{
	if (!ValidateStageDefinition(stage).IsValid())
	{
		return false;
	}

	StageDefinition configuredStage = ApplyDifficulty(stage, difficulty);
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
	for (std::size_t index = 0; index < preview.points.size(); ++index)
	{
		const float speed = (std::max)(velocity.Length(), COLLISION_EPSILON);
		const float timeScale = AIM_PREVIEW_STEP_LENGTH / (force * speed);
		velocity.y += AIM_PREVIEW_GRAVITY * timeScale;
		position += velocity.Normalized() * AIM_PREVIEW_STEP_LENGTH;

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

		auto targetPosition = _targetBallList._targetBallList.GetHeadPosition();
		while (targetPosition != nullptr)
		{
			const TargetBall& target =
				_targetBallList._targetBallList.GetNext(targetPosition);
			const Vector2 offset = position - target.position;
			const float collisionRadius = _ball.GetSize() + target.size;
			const float distanceSquared = offset.LengthSquared();
			if (distanceSquared > collisionRadius * collisionRadius)
			{
				continue;
			}

			const float distance = std::sqrt(distanceSquared);
			Vector2 normal = distance > COLLISION_EPSILON
				? offset / distance
				: (velocity * -1.0f).Normalized();
			if (normal.LengthSquared() == 0.0f)
			{
				normal = { 0.0f, -1.0f };
			}
			position += normal * (collisionRadius - distance + COLLISION_EPSILON);
			velocity = ReflectVelocity(velocity, normal, _pegRestitution);
			if (!preview.PredictsPegCollision())
			{
				preview.firstPegCollisionPoint = index;
			}
			break;
		}

		preview.points[index] = position;
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

void GameWorld::RestoreRemovedPegs(Vector2 triggerPosition)
{
	constexpr float POSITION_EPSILON_SQUARED = 0.0001f;
	int restoredPegs = 0;
	for (const PegDefinition& definition : _stage.pegLayout.pegs)
	{
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
		triggerPosition,
		PegType::Refresh,
		0,
		_score.currentCombo,
		restoredPegs,
		0.0f
	});
}

bool GameWorld::EnsureRefreshPegAfterTurn()
{
	TargetBall* replacement = nullptr;
	auto position = _targetBallList._targetBallList.GetHeadPosition();
	while (position != nullptr)
	{
		TargetBall& active = _targetBallList._targetBallList.GetNext(position);
		if (active.type == PegType::Refresh)
		{
			return false;
		}
		if (replacement == nullptr
			|| (replacement->type != PegType::Normal && active.type == PegType::Normal))
		{
			replacement = &active;
		}
	}

	Vector2 refreshPosition = GameLayout::BallInitialPosition;
	if (replacement != nullptr)
	{
		replacement->type = PegType::Refresh;
		refreshPosition = replacement->position;
	}
	else if (!_stage.pegLayout.pegs.empty())
	{
		TargetBall restored;
		restored.setting(_stage.pegLayout.pegs.front());
		restored.type = PegType::Refresh;
		refreshPosition = restored.position;
		_targetBallList.add(restored);
	}
	else
	{
		return false;
	}

	_events.push_back({
		GameEventType::RefreshGuaranteed,
		refreshPosition,
		PegType::Refresh,
		0,
		_score.currentCombo,
		1,
		0.0f
	});
	return true;
}

std::size_t GameWorld::GetLivingEnemyCount() const noexcept
{
	return static_cast<std::size_t>(std::count_if(
		_enemies.begin(),
		_enemies.end(),
		[](const EnemyCombatant& enemy)
		{
			return enemy.IsAlive();
		}));
}

bool GameWorld::SelectNextLivingEnemy() noexcept
{
	for (std::size_t index = _activeEnemyIndex + 1; index < _enemies.size(); ++index)
	{
		if (_enemies[index].IsAlive())
		{
			_activeEnemyIndex = index;
			return true;
		}
	}
	return false;
}

EnemyActionDefinition GameWorld::GetNextEnemyAction() const noexcept
{
	const Enemy& enemy = GetActiveEnemy().actor;
	if (!_stage.enemyPattern.empty())
	{
		const std::size_t actionIndex = static_cast<std::size_t>(enemy.GetCount())
			% _stage.enemyPattern.size();
		return _stage.enemyPattern[actionIndex];
	}

	if (enemy.GetCount() < _stage.rules.enemyStepsBeforeAttack)
	{
		return { EnemyActionType::Advance, _stage.rules.enemyStep };
	}
	return { EnemyActionType::Strike, _stage.rules.playerDamage };
}

void GameWorld::ExecuteEnemyAction(const EnemyActionDefinition& action)
{
	EnemyCombatant& activeEnemy = GetActiveEnemy();
	Enemy& enemy = activeEnemy.actor;
	const float enemyY = _enemies.size() > 1
		? GameLayout::EnemyGroupY
		: GameLayout::EnemyInitialPosition.y;
	switch (action.type)
	{
	case EnemyActionType::Advance:
		enemy.SetX(enemy.GetX() - action.magnitude);
		_events.push_back({
			GameEventType::EnemyAdvanced,
			{ enemy.GetX(), enemyY },
			PegType::Normal,
			0,
			0,
			0,
			action.magnitude
		});
		break;
	case EnemyActionType::Strike:
	{
		const ProgressionModifiers modifiers = _loadout.CalculateModifiers();
		const float incomingDamage = action.magnitude
			* modifiers.incomingDamageMultiplier;
		_player.SetHp(_player.GetHp() - incomingDamage);
		_feedback.lastPlayerDamage = incomingDamage;
		break;
	}
	case EnemyActionType::Fortify:
		activeEnemy.shield += action.magnitude;
		_events.push_back({
			GameEventType::EnemyFortified,
			{ enemy.GetX(), enemyY },
			PegType::Normal,
			0,
			0,
			0,
			action.magnitude
		});
		break;
	}
}

void GameWorld::ResolveTurn()
{
	EnemyCombatant& activeEnemy = GetActiveEnemy();
	Enemy& enemy = activeEnemy.actor;
	const float enemyY = _enemies.size() > 1
		? GameLayout::EnemyGroupY
		: GameLayout::EnemyInitialPosition.y;
	const OrbDefinition& attackOrb = _loadout.GetSelectedOrb();
	std::vector<std::size_t> attackTargets;
	if (attackOrb.attackTarget == AttackTarget::All)
	{
		for (std::size_t index = 0; index < _enemies.size(); ++index)
		{
			if (_enemies[index].IsAlive())
			{
				attackTargets.push_back(index);
			}
		}
	}
	else
	{
		attackTargets.push_back(_activeEnemyIndex);
	}

	float totalEnemyDamage = 0.0f;
	std::vector<std::size_t> defeatedEnemies;
	for (const std::size_t targetIndex : attackTargets)
	{
		EnemyCombatant& target = _enemies[targetIndex];
		const float modifiedDamage = _pendingDamage * target.definition.damageTakenMultiplier;
		const float shieldAbsorbed = (std::min)(target.shield, modifiedDamage);
		target.shield -= shieldAbsorbed;
		const float appliedDamage = modifiedDamage - shieldAbsorbed;
		target.actor.SetHp(target.actor.GetHp() - appliedDamage);
		totalEnemyDamage += appliedDamage;
		if (!target.IsAlive())
		{
			defeatedEnemies.push_back(targetIndex);
		}
	}
	_feedback.lastEnemyDamage = totalEnemyDamage;
	_feedback.lastPlayerDamage = 0.0f;
	_score.lastTurn = _score.currentShot;
	_score.total += _score.lastTurn;
	_score.currentShot = 0;
	_score.currentCombo = 0;

	const bool enemyDefeated = enemy.GetHp() <= 0.0f;
	if (!enemyDefeated)
	{
		ExecuteEnemyAction(GetNextEnemyAction());
	}
	enemy.SetCount(enemy.GetCount() + 1);
	++_completedTurns;
	_feedback.turnNumber = _completedTurns;
	_events.push_back({
		GameEventType::PlayerAttack,
		{ enemy.GetX(), enemyY },
		PegType::Normal,
		0,
		0,
		static_cast<int>(attackTargets.size()),
		totalEnemyDamage,
		attackOrb.attackDelivery,
		attackOrb.attackTarget,
		_activeEnemyIndex
	});
	_events.push_back({
		GameEventType::TurnResolved,
		{},
		PegType::Normal,
		_score.lastTurn,
		_score.bestCombo,
		_feedback.currentShotPegHits,
		_feedback.lastEnemyDamage
	});
	for (const std::size_t defeatedIndex : defeatedEnemies)
	{
		const EnemyCombatant& defeated = _enemies[defeatedIndex];
		_events.push_back({
			GameEventType::EnemyDefeated,
			{ defeated.actor.GetX(), enemyY },
			PegType::Normal,
			0,
			0,
			static_cast<int>(GetLivingEnemyCount()),
			totalEnemyDamage
		});
	}
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
	_loadout.AdvanceOrb();
	EnsureRefreshPegAfterTurn();

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
	else if (GetLivingEnemyCount() == 0)
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
		if (enemyDefeated)
		{
			SelectNextLivingEnemy();
		}
		TransitionTo(GameState::Aiming);
		_feedback.type = _feedback.lastPlayerDamage > 0.0f
			? GameFeedbackType::PlayerDamaged
			: GameFeedbackType::TurnResolved;
	}
}

std::uint32_t GameWorld::GetBattleShuffleSeed() const noexcept
{
	std::uint32_t hash = 2166136261u;
	for (const unsigned char character : _stage.id)
	{
		hash ^= character;
		hash *= 16777619u;
	}
	return hash;
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
		_completedTurns
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
