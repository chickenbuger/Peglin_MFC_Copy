#pragma once

#include "PegLayout.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class EnemyActionType
{
	Advance,
	Strike,
	Fortify
};

struct EnemyActionDefinition
{
	EnemyActionType type = EnemyActionType::Advance;
	float magnitude = 0.0f;
};

enum class EnemyVisualKind
{
	CrystalToad,
	EmberBat,
	MossShaman
};

struct EnemyDefinition
{
	std::string id;
	std::string displayName;
	EnemyVisualKind visual = EnemyVisualKind::CrystalToad;
	float health = 20.0f;
};

struct StageRules
{
	float playerHealth = 100.0f;
	float enemyHealth = 20.0f;
	float playerDamage = 20.0f;
	int enemyStepsBeforeAttack = 8;
	float enemyStep = 64.0f;
	float pegRestitution = 0.85f;
};

struct StageDefinition
{
	std::string id;
	std::string displayName;
	PegLayoutDefinition pegLayout;
	StageRules rules;
	bool isBoss = false;
	std::vector<EnemyDefinition> enemies;
	std::vector<EnemyActionDefinition> enemyPattern;
};

enum class StageLoadError
{
	None,
	NotFound,
	EmptyId,
	EmptyPegLayout,
	TooManyPegs,
	PegOutOfBounds,
	DuplicatePegPosition,
	InvalidPegType,
	InvalidPlayerHealth,
	InvalidEnemyHealth,
	InvalidPlayerDamage,
	InvalidEnemySteps,
	InvalidEnemyStep,
	InvalidPegRestitution,
	MissingBossPattern,
	InvalidEnemyAction,
	InvalidEnemyActionMagnitude,
	TooManyEnemies,
	InvalidEnemyId,
	DuplicateEnemyId,
	InvalidEnemyName,
	InvalidEnemyVisual,
	InvalidEnemyRosterHealth
};

struct StageValidationResult
{
	StageLoadError error = StageLoadError::None;
	std::size_t pegIndex = 0;

	bool IsValid() const noexcept { return error == StageLoadError::None; }
};

struct StageLoadResult
{
	std::optional<StageDefinition> stage;
	StageValidationResult validation;

	bool IsSuccess() const noexcept { return stage.has_value() && validation.IsValid(); }
};

struct StageCatalogEntry
{
	std::string_view id;
	std::string_view displayName;
};

StageDefinition CreateDefaultStageDefinition();
StageDefinition CreateChallengeStageDefinition();
StageDefinition CreateBossStageDefinition();
StageValidationResult ValidateStageDefinition(const StageDefinition& stage) noexcept;
StageLoadResult LoadStageDefinition(std::string_view stageId);
const std::array<StageCatalogEntry, 3>& GetStageCatalog() noexcept;
