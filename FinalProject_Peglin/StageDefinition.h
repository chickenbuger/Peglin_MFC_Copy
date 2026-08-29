#pragma once

#include "PegLayout.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

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
	InvalidPegRestitution
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

StageDefinition CreateDefaultStageDefinition();
StageDefinition CreateChallengeStageDefinition();
StageValidationResult ValidateStageDefinition(const StageDefinition& stage) noexcept;
StageLoadResult LoadStageDefinition(std::string_view stageId);
