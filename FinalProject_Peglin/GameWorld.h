#pragma once

#include "Enemy.h"
#include "GameState.h"
#include "Parent_ball.h"
#include "PegLayout.h"
#include "Player.h"
#include "TargetBall.h"

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

class GameWorld
{
public:
	explicit GameWorld(PegLayoutDefinition pegLayout = CreateDefaultPegLayout());

	GameUpdateResult Update(float deltaSeconds);
	void ResetGame();
	void ResetBallToAiming();

	bool BeginAim(Vector2 position);
	void UpdateAim(Vector2 position);
	bool ReleaseShot(Vector2 position);
	void CancelAim();
	bool TogglePause();
	void SetPegRestitution(float restitution) noexcept;
	float GetPegRestitution() const noexcept { return _pegRestitution; }

	Player& GetPlayer() noexcept { return _player; }
	Enemy& GetEnemy() noexcept { return _enemy; }
	Parent_ball& GetBall() noexcept { return _ball; }
	TargetBallList& GetTargets() noexcept { return _targetBallList; }
	const PegLayoutDefinition& GetPegLayout() const noexcept { return _pegLayout; }
	GameState GetState() const noexcept { return _gameState; }
	const GameFeedback& GetFeedback() const noexcept { return _feedback; }
	const GameScore& GetScore() const noexcept { return _score; }

private:
	void InitializeTargets();
	void HandlePegCollisions();
	void AwardPeg(const TargetBall& target);
	void ApplyBombEffect(const TargetBall& bomb);
	void RestoreRemovedPegs(Vector2 excludedPosition);
	void ResolveTurn();
	GameUpdateResult ReportTerminalResult(GameUpdateResult result) noexcept;
	bool TransitionTo(GameState nextState);

	Player _player;
	Enemy _enemy;
	Parent_ball _ball;
	TargetBallList _targetBallList;
	PegLayoutDefinition _pegLayout;
	float _pendingDamage = 0.0f;
	float _pegRestitution = 0.85f;
	GameFeedback _feedback;
	GameScore _score;
	GameState _gameState = GameState::Aiming;
	GameState _stateBeforePause = GameState::Aiming;
	bool _terminalResultReported = false;
};
