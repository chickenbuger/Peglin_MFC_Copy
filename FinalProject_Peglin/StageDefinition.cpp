#include "pch.h"
#include "StageDefinition.h"

#include "GameLayout.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <utility>

namespace
{
	constexpr std::size_t MAX_STAGE_PEGS = 256;
	constexpr std::size_t MAX_STAGE_ENEMIES = 3;
	constexpr float DUPLICATE_POSITION_EPSILON_SQUARED = 0.0001f;
	constexpr std::array<StageCatalogEntry, 8> STAGE_CATALOG = {
		StageCatalogEntry{ "stage-1", "Forgotten Forest" },
		StageCatalogEntry{ "stage-2", "Dense Cavern" },
		StageCatalogEntry{ "stage-4", "Thornwood Thicket" },
		StageCatalogEntry{ "stage-5", "Fungal Hollow" },
		StageCatalogEntry{ "stage-6", "Crystal Grotto" },
		StageCatalogEntry{ "stage-7", "Ember Roost" },
		StageCatalogEntry{ "stage-8", "Shaman Mire" },
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

	bool IsKnownEnemyVisual(EnemyVisualKind visual) noexcept
	{
		switch (visual)
		{
		case EnemyVisualKind::CrystalToad:
		case EnemyVisualKind::EmberBat:
		case EnemyVisualKind::MossShaman:
			return true;
		default:
			return false;
		}
	}

	bool IsSafeEnemyId(std::string_view id) noexcept
	{
		if (id.empty() || id.size() > 48)
		{
			return false;
		}
		for (const char character : id)
		{
			const bool lower = character >= 'a' && character <= 'z';
			const bool digit = character >= '0' && character <= '9';
			if (!lower && !digit && character != '-')
			{
				return false;
			}
		}
		return true;
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

	StageDefinition CreateRouteStage(
		std::string id,
		std::string displayName,
		int columns,
		int rows,
		Vector2 start,
		float spacing,
		float jitter,
		std::uint32_t seed,
		float enemyHealth,
		float playerDamage,
		int enemySteps,
		float enemyStep,
		float restitution,
		std::vector<EnemyDefinition> enemies,
		std::initializer_list<std::pair<std::size_t, PegType>> pegTypes)
	{
		StageDefinition stage;
		stage.id = std::move(id);
		stage.displayName = std::move(displayName);
		stage.pegLayout = CreateSeededPegLayout(
			columns, rows, start, spacing, jitter, seed);
		for (const auto& [index, type] : pegTypes)
		{
			if (index < stage.pegLayout.pegs.size())
			{
				stage.pegLayout.pegs[index].type = type;
			}
		}
		stage.rules.playerHealth = 100.0f;
		stage.rules.enemyHealth = enemyHealth;
		stage.rules.playerDamage = playerDamage;
		stage.rules.enemyStepsBeforeAttack = enemySteps;
		stage.rules.enemyStep = enemyStep;
		stage.rules.pegRestitution = restitution;
		stage.enemies = std::move(enemies);
		return stage;
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

std::vector<StageDefinition> CreateBuiltInStageDefinitions()
{
	std::vector<StageDefinition> stages;
	stages.reserve(STAGE_CATALOG.size());
	stages.push_back(CreateDefaultStageDefinition());
	stages.push_back(CreateChallengeStageDefinition());
	stages.push_back(CreateRouteStage(
		"stage-4", "Thornwood Thicket",
		11, 4, { 60.0f, 390.0f }, 82.0f, 10.0f, 20260901u,
		24.0f, 22.0f, 7, 60.0f, 0.86f,
		{
			{ "moss-shaman", "Moss Shaman", EnemyVisualKind::MossShaman, 8.0f },
			{ "crystal-toad", "Crystal Toad", EnemyVisualKind::CrystalToad, 9.0f },
			{ "ember-bat", "Ember Bat", EnemyVisualKind::EmberBat, 7.0f }
		},
		{ { 6, PegType::Critical }, { 17, PegType::Bomb }, { 28, PegType::Refresh }, { 38, PegType::Critical } }));
	stages.push_back(CreateRouteStage(
		"stage-5", "Fungal Hollow",
		9, 5, { 130.0f, 350.0f }, 80.0f, 14.0f, 20260902u,
		28.0f, 23.0f, 7, 58.0f, 0.88f,
		{
			{ "moss-shaman-elder", "Moss Shaman Elder", EnemyVisualKind::MossShaman, 11.0f },
			{ "ember-bat", "Ember Bat", EnemyVisualKind::EmberBat, 8.0f },
			{ "crystal-toad", "Crystal Toad", EnemyVisualKind::CrystalToad, 9.0f }
		},
		{ { 3, PegType::Critical }, { 12, PegType::Refresh }, { 23, PegType::Bomb }, { 34, PegType::Critical }, { 41, PegType::Bomb } }));
	stages.push_back(CreateRouteStage(
		"stage-6", "Crystal Grotto",
		10, 4, { 85.0f, 385.0f }, 88.0f, 16.0f, 20260903u,
		32.0f, 24.0f, 6, 56.0f, 0.91f,
		{
			{ "crystal-toad-guard", "Crystal Toad Guard", EnemyVisualKind::CrystalToad, 13.0f },
			{ "ember-bat-scout", "Ember Bat Scout", EnemyVisualKind::EmberBat, 9.0f },
			{ "moss-shaman", "Moss Shaman", EnemyVisualKind::MossShaman, 10.0f }
		},
		{ { 5, PegType::Critical }, { 14, PegType::Bomb }, { 20, PegType::Refresh }, { 30, PegType::Bomb }, { 37, PegType::Critical } }));
	stages.push_back(CreateRouteStage(
		"stage-7", "Ember Roost",
		11, 4, { 55.0f, 380.0f }, 83.0f, 18.0f, 20260904u,
		36.0f, 26.0f, 5, 54.0f, 0.93f,
		{
			{ "ember-bat", "Ember Bat", EnemyVisualKind::EmberBat, 10.0f },
			{ "ember-bat-scout", "Ember Bat Scout", EnemyVisualKind::EmberBat, 11.0f },
			{ "crystal-toad", "Crystal Toad", EnemyVisualKind::CrystalToad, 15.0f }
		},
		{ { 4, PegType::Critical }, { 10, PegType::Bomb }, { 19, PegType::Refresh }, { 27, PegType::Bomb }, { 35, PegType::Critical }, { 42, PegType::Bomb } }));
	stages.push_back(CreateRouteStage(
		"stage-8", "Shaman Mire",
		9, 5, { 125.0f, 345.0f }, 81.0f, 18.0f, 20260905u,
		40.0f, 27.0f, 5, 52.0f, 0.94f,
		{
			{ "moss-shaman", "Moss Shaman", EnemyVisualKind::MossShaman, 11.0f },
			{ "moss-shaman-elder", "Moss Shaman Elder", EnemyVisualKind::MossShaman, 14.0f },
			{ "crystal-toad-guard", "Crystal Toad Guard", EnemyVisualKind::CrystalToad, 15.0f }
		},
		{ { 2, PegType::Critical }, { 11, PegType::Refresh }, { 18, PegType::Bomb }, { 26, PegType::Critical }, { 33, PegType::Refresh }, { 40, PegType::Bomb } }));
	stages.push_back(CreateBossStageDefinition());
	return stages;
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
	if (stage.enemies.size() > MAX_STAGE_ENEMIES)
	{
		return { StageLoadError::TooManyEnemies, stage.enemies.size() };
	}
	for (std::size_t index = 0; index < stage.enemies.size(); ++index)
	{
		const EnemyDefinition& enemy = stage.enemies[index];
		if (!IsSafeEnemyId(enemy.id))
		{
			return { StageLoadError::InvalidEnemyId, index };
		}
		if (enemy.displayName.empty() || enemy.displayName.size() > 64)
		{
			return { StageLoadError::InvalidEnemyName, index };
		}
		if (!IsKnownEnemyVisual(enemy.visual))
		{
			return { StageLoadError::InvalidEnemyVisual, index };
		}
		if (!IsFinitePositive(enemy.health))
		{
			return { StageLoadError::InvalidEnemyRosterHealth, index };
		}
		if (!std::isfinite(enemy.damageTakenMultiplier)
			|| enemy.damageTakenMultiplier < 0.1f
			|| enemy.damageTakenMultiplier > 5.0f)
		{
			return { StageLoadError::InvalidEnemyDamageTakenMultiplier, index };
		}
		for (std::size_t earlier = 0; earlier < index; ++earlier)
		{
			if (stage.enemies[earlier].id == enemy.id)
			{
				return { StageLoadError::DuplicateEnemyId, index };
			}
		}
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
			|| peg.position.x < GameLayout::PegFieldLeft + GameLayout::PegRadius
			|| peg.position.x > GameLayout::PegFieldRight - GameLayout::PegRadius
			|| peg.position.y < GameLayout::PegFieldTop + GameLayout::PegRadius
			|| peg.position.y > GameLayout::PegFieldBottom - GameLayout::PegRadius)
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
	for (StageDefinition& stage : CreateBuiltInStageDefinitions())
	{
		if (stage.id == stageId)
		{
			return ValidateLoadedStage(std::move(stage));
		}
	}

	StageLoadResult result;
	result.validation.error = StageLoadError::NotFound;
	return result;
}

const std::array<StageCatalogEntry, 8>& GetStageCatalog() noexcept
{
	return STAGE_CATALOG;
}
