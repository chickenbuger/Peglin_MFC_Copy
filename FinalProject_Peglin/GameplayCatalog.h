#pragma once

#include "Progression.h"
#include "StageDefinition.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

enum class GameplayCatalogLoadState
{
	External,
	BuiltInFallback
};

enum class GameplayCatalogLoadError
{
	None,
	MissingFile,
	IoFailure,
	FileTooLarge,
	InvalidEncoding,
	UnsupportedVersion,
	UnexpectedSection,
	UnknownKey,
	DuplicateKey,
	MissingField,
	InvalidValue,
	DuplicateId,
	UnknownEffectReference,
	CircularEffectReference,
	UnknownEnemyReference,
	InvalidResolvedEffect
};

struct EnemyEffectBinding
{
	std::string enemyId;
	float damageTakenMultiplier = 1.0f;
};

struct GameplayCatalog
{
	ProgressionCatalog progression;
	std::vector<EnemyEffectBinding> enemies;
};

struct GameplayCatalogLoadResult
{
	GameplayCatalogLoadState state = GameplayCatalogLoadState::BuiltInFallback;
	GameplayCatalogLoadError error = GameplayCatalogLoadError::None;
	std::size_t errorLine = 0;
	GameplayCatalog catalog;

	bool UsedExternalContent() const noexcept
	{
		return state == GameplayCatalogLoadState::External
			&& error == GameplayCatalogLoadError::None;
	}
};

GameplayCatalogLoadResult LoadGameplayCatalog(
	const std::filesystem::path& path,
	const std::vector<StageDefinition>& stages);
bool ActivateGameplayCatalog(
	const GameplayCatalogLoadResult& result,
	std::vector<StageDefinition>& stages);
