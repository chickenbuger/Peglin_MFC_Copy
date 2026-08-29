#pragma once

#include "Enemy.h"
#include "GameOptions.h"
#include "GameState.h"
#include "Parent_ball.h"
#include "PegLayout.h"
#include "Player.h"
#include "Progression.h"
#include "StageDefinition.h"
#include "TargetBall.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class GameUpdateResult
{
	None,
	Victory,
	Defeat
};

enum class GameFeedbackType
{
	Ready,
	ShotLaunched,
	PegHit,
	TurnResolved,
	EnemyAdvanced,
	EnemyFortified,
	PlayerDamaged,
	Victory,
	Defeat,
	Paused
};

struct GameFeedback
{
	GameFeedbackType type = GameFeedbackType::Ready;
	int currentShotPegHits = 0;
	int turnNumber = 0;
	float lastEnemyDamage = 0.0f;
	float lastPlayerDamage = 0.0f;
};

struct GameScore
{
	int total = 0;
	int currentShot = 0;
	int lastTurn = 0;
	int currentCombo = 0;
	int bestCombo = 0;
};

enum class GameEventType
{
	PegHit,
	BombTriggered,
	RefreshTriggered,
	TurnResolved,
	EnemyAdvanced,
	EnemyFortified,
	PlayerDamaged,
	Victory,
	Defeat
};

struct GameEvent
{
	GameEventType type = GameEventType::PegHit;
	Vector2 position;
	PegType pegType = PegType::Normal;
	int scoreAwarded = 0;
	int combo = 0;
	int affectedPegs = 0;
	float damage = 0.0f;
};

struct AimPreview
{
	static constexpr std::size_t PointCount = 16;

	bool visible = false;
	Vector2 launchDirection;
	float dragDistance = 0.0f;
	float normalizedStrength = 0.0f;
	std::array<Vector2, PointCount> points{};
};

struct GameResultSummary
{
	GameUpdateResult result = GameUpdateResult::None;
	std::string stageId;
	std::string stageName;
	int totalScore = 0;
	int bestCombo = 0;
	int turns = 0;
};

class GameWorld
{
public:
	GameWorld();
	explicit GameWorld(PegLayoutDefinition pegLayout);
	explicit GameWorld(StageDefinition stage);

	GameUpdateResult Update(float deltaSeconds);
	void ResetGame();
	void ResetBallToAiming();
	bool LoadStage(
		std::string_view stageId,
		GameDifficulty difficulty = GameDifficulty::Normal);
	bool SelectOrb(std::string_view orbId) { return _loadout.SelectOrb(orbId); }
	bool AcquireRelic(std::string_view relicId) { return _loadout.AcquireRelic(relicId); }
	void ResetProgression();

	bool BeginAim(Vector2 position);
	void UpdateAim(Vector2 position);
	AimPreview GetAimPreview() const noexcept;
	bool ReleaseShot(Vector2 position);
	void CancelAim();
	bool TogglePause();
	void SetPegRestitution(float restitution) noexcept;
	float GetPegRestitution() const noexcept { return _pegRestitution; }

	Player& GetPlayer() noexcept { return _player; }
	Enemy& GetEnemy() noexcept { return _enemy; }
	Parent_ball& GetBall() noexcept { return _ball; }
	TargetBallList& GetTargets() noexcept { return _targetBallList; }
	const PegLayoutDefinition& GetPegLayout() const noexcept { return _stage.pegLayout; }
	const StageDefinition& GetStage() const noexcept { return _stage; }
	GameDifficulty GetDifficulty() const noexcept { return _difficulty; }
	GameState GetState() const noexcept { return _gameState; }
	const GameFeedback& GetFeedback() const noexcept { return _feedback; }
	const GameScore& GetScore() const noexcept { return _score; }
	const PlayerLoadout& GetLoadout() const noexcept { return _loadout; }
	ProgressionModifiers GetProgressionModifiers() const noexcept
	{
		return _loadout.CalculateModifiers();
	}
	EnemyActionDefinition GetNextEnemyAction() const noexcept;
	float GetEnemyShield() const noexcept { return _enemyShield; }
	std::vector<GameEvent> ConsumeEvents();
	std::optional<GameResultSummary> GetResultSummary() const;

private:
	void InitializeTargets();
	void HandlePegCollisions();
	void AwardPeg(const TargetBall& target);
	void ApplyBombEffect(const TargetBall& bomb);
	void RestoreRemovedPegs(Vector2 excludedPosition);
	void ExecuteEnemyAction(const EnemyActionDefinition& action);
	void ResolveTurn();
	GameUpdateResult ReportTerminalResult(GameUpdateResult result) noexcept;
	bool TransitionTo(GameState nextState);

	Player _player;
	Enemy _enemy;
	Parent_ball _ball;
	TargetBallList _targetBallList;
	StageDefinition _stage;
	GameDifficulty _difficulty = GameDifficulty::Normal;
	float _pendingDamage = 0.0f;
	float _enemyShield = 0.0f;
	float _pegRestitution = 0.85f;
	GameFeedback _feedback;
	GameScore _score;
	PlayerLoadout _loadout;
	std::vector<GameEvent> _events;
	Vector2 _aimStart;
	Vector2 _aimCurrent;
	bool _aimInProgress = false;
	GameState _gameState = GameState::Aiming;
	GameState _stateBeforePause = GameState::Aiming;
	bool _terminalResultReported = false;
};
