#pragma once

#include "StageDefinition.h"

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <vector>

enum class ContentLoadState
{
	External,
	BuiltInFallback
};

enum class ContentLoadError
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
	DuplicateStageId,
	StageValidationFailed
};

struct ContentLoadResult
{
	ContentLoadState state = ContentLoadState::BuiltInFallback;
	ContentLoadError error = ContentLoadError::None;
	std::size_t errorLine = 0;
	std::vector<StageDefinition> stages;

	bool UsedExternalContent() const noexcept
	{
		return state == ContentLoadState::External && error == ContentLoadError::None;
	}
};

std::vector<StageDefinition> CreateBuiltInContentCatalog();
ContentLoadResult LoadContentCatalog(const std::filesystem::path& path);
const StageDefinition* FindContentStage(
	const std::vector<StageDefinition>& stages,
	std::string_view stageId) noexcept;
