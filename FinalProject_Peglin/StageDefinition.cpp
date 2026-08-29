#include "pch.h"
#include "StageDefinition.h"

#include "GameLayout.h"

#include <cmath>
#include <utility>

namespace
{
	constexpr std::size_t MAX_STAGE_PEGS = 256;
	constexpr float DUPLICATE_POSITION_EPSILON_SQUARED = 0.0001f;
	constexpr std::array<StageCatalogEntry, 3> STAGE_CATALOG = {
		StageCatalogEntry{ "stage-1", "Forgotten Forest" },
		StageCatalogEntry{ "stage-2", "Dense Cavern" },
		StageCatalogEntry{ "stage-3", "Rootbound Citadel" }
	};

	bool IsFinitePositive(float value) noexcept
	{
		return std::isfinite(value) && value > 0.0f;
	}

	bool IsKnownPegType(PegType type) noexcept
	{
		switch (type)
		{
		case PegType::Normal:
		case PegType::Critical:
		case PegType::Bomb:
		case PegType::Refresh:
			return true;
		default:
			return false;
		}
	}

	bool IsKnownEnemyAction(EnemyActionType type) noexcept
	{
		switch (type)
		{
		case EnemyActionType::Advance:
		case EnemyActionType::Strike:
		case EnemyActionType::Fortify:
			return true;
		default:
			return false;
		}
	}

	StageLoadResult ValidateLoadedStage(StageDefinition stage)
	{
		StageLoadResult result;
		result.validation = ValidateStageDefinition(stage);
		if (result.validation.IsValid())
		{
			result.stage = std::move(stage);
		}
		return result;
	}
}

StageDefinition CreateDefaultStageDefinition()
{
	StageDefinition stage;
	stage.id = "stage-1";
	stage.displayName = "Forgotten Forest";
	stage.pegLayout = CreateDefaultPegLayout();
	stage.rules = {};
	return stage;
}

StageDefinition CreateChallengeStageDefinition()
{
	StageDefinition stage;
	stage.id = "stage-2";
	stage.displayName = "Dense Cavern";
	stage.pegLayout = CreateSeededPegLayout(
		10,
		4,
		{ 80.0f, 380.0f },
		90.0f,
		12.0f,
		20260829u);

	if (stage.pegLayout.pegs.size() == 40)
	{
		stage.pegLayout.pegs[7].type = PegType::Critical;
		stage.pegLayout.pegs[16].type = PegType::Bomb;
		stage.pegLayout.pegs[24].type = PegType::Refresh;
		stage.pegLayout.pegs[33].type = PegType::Critical;
	}

	stage.rules.playerHealth = 100.0f;
	stage.rules.enemyHealth = 30.0f;
	stage.rules.playerDamage = 25.0f;
	stage.rules.enemyStepsBeforeAttack = 6;
	stage.rules.enemyStep = 64.0f;
	stage.rules.pegRestitution = 0.9f;
	return stage;
}

StageDefinition CreateBossStageDefinition()
{
	StageDefinition stage;
	stage.id = "stage-3";
	stage.displayName = "Rootbound Citadel";
	stage.pegLayout = CreateSeededPegLayout(
		9,
		4,
		{ 100.0f, 380.0f },
		100.0f,
		16.0f,
		20260830u);

	if (stage.pegLayout.pegs.size() == 36)
	{
		stage.pegLayout.pegs[4].type = PegType::Critical;
		stage.pegLayout.pegs[10].type = PegType::Bomb;
		stage.pegLayout.pegs[17].type = PegType::Refresh;
		stage.pegLayout.pegs[22].type = PegType::Bomb;
		stage.pegLayout.pegs[31].type = PegType::Critical;
	}

	stage.rules.playerHealth = 110.0f;
	stage.rules.enemyHealth = 60.0f;
	stage.rules.playerDamage = 20.0f;
	stage.rules.enemyStepsBeforeAttack = 4;
	stage.rules.enemyStep = 48.0f;
	stage.rules.pegRestitution = 0.92f;
	stage.isBoss = true;
	stage.enemyPattern = {
		{ EnemyActionType::Advance, 48.0f },
		{ EnemyActionType::Fortify, 4.0f },
		{ EnemyActionType::Strike, 18.0f },
		{ EnemyActionType::Strike, 24.0f }
	};
	return stage;
}

StageValidationResult ValidateStageDefinition(const StageDefinition& stage) noexcept
{
	if (stage.id.empty())
	{
		return { StageLoadError::EmptyId, 0 };
	}
	if (stage.pegLayout.pegs.empty())
	{
		return { StageLoadError::EmptyPegLayout, 0 };
	}
	if (stage.pegLayout.pegs.size() > MAX_STAGE_PEGS)
	{
		return { StageLoadError::TooManyPegs, stage.pegLayout.pegs.size() };
	}
	if (!IsFinitePositive(stage.rules.playerHealth))
	{
		return { StageLoadError::InvalidPlayerHealth, 0 };
	}
	if (!IsFinitePositive(stage.rules.enemyHealth))
	{
		return { StageLoadError::InvalidEnemyHealth, 0 };
	}
	if (!IsFinitePositive(stage.rules.playerDamage))
	{
		return { StageLoadError::InvalidPlayerDamage, 0 };
	}
	if (stage.rules.enemyStepsBeforeAttack <= 0)
	{
		return { StageLoadError::InvalidEnemySteps, 0 };
	}
	if (!IsFinitePositive(stage.rules.enemyStep))
	{
		return { StageLoadError::InvalidEnemyStep, 0 };
	}
	if (!std::isfinite(stage.rules.pegRestitution)
		|| stage.rules.pegRestitution < 0.0f
		|| stage.rules.pegRestitution > 1.0f)
	{
		return { StageLoadError::InvalidPegRestitution, 0 };
	}
	if (stage.isBoss && stage.enemyPattern.empty())
	{
		return { StageLoadError::MissingBossPattern, 0 };
	}
	for (std::size_t index = 0; index < stage.enemyPattern.size(); ++index)
	{
		const EnemyActionDefinition& action = stage.enemyPattern[index];
		if (!IsKnownEnemyAction(action.type))
		{
			return { StageLoadError::InvalidEnemyAction, index };
		}
		if (!IsFinitePositive(action.magnitude))
		{
			return { StageLoadError::InvalidEnemyActionMagnitude, index };
		}
	}

	for (std::size_t index = 0; index < stage.pegLayout.pegs.size(); ++index)
	{
		const PegDefinition& peg = stage.pegLayout.pegs[index];
		if (!IsKnownPegType(peg.type))
		{
			return { StageLoadError::InvalidPegType, index };
		}
		if (!std::isfinite(peg.position.x)
			|| !std::isfinite(peg.position.y)
			|| peg.position.x < GameLayout::BoardLeft + GameLayout::PegRadius
			|| peg.position.x > GameLayout::BoardRight - GameLayout::PegRadius
			|| peg.position.y < GameLayout::BoardTop + GameLayout::PegRadius
			|| peg.position.y > GameLayout::BoardBottom - GameLayout::PegRadius)
		{
			return { StageLoadError::PegOutOfBounds, index };
		}

		for (std::size_t earlier = 0; earlier < index; ++earlier)
		{
			if ((peg.position - stage.pegLayout.pegs[earlier].position).LengthSquared()
				<= DUPLICATE_POSITION_EPSILON_SQUARED)
			{
				return { StageLoadError::DuplicatePegPosition, index };
			}
		}
	}

	return {};
}

StageLoadResult LoadStageDefinition(std::string_view stageId)
{
	if (stageId == "stage-1")
	{
		return ValidateLoadedStage(CreateDefaultStageDefinition());
	}
	if (stageId == "stage-2")
	{
		return ValidateLoadedStage(CreateChallengeStageDefinition());
	}
	if (stageId == "stage-3")
	{
		return ValidateLoadedStage(CreateBossStageDefinition());
	}

	StageLoadResult result;
	result.validation.error = StageLoadError::NotFound;
	return result;
}

const std::array<StageCatalogEntry, 3>& GetStageCatalog() noexcept
{
	return STAGE_CATALOG;
}
