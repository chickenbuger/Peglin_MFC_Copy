#pragma once

#include "ContentCatalog.h"
#include "DifficultyCurve.h"
#include "GameplayCatalog.h"
#include "Progression.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

inline bool HasContentPreviewCommandLineFlag(std::wstring_view commandLine) noexcept
{
	constexpr std::wstring_view flag = L"--content-preview";
	std::size_t position = commandLine.find(flag);
	while (position != std::wstring_view::npos)
	{
		const bool validBefore = position == 0
			|| commandLine[position - 1] == L' '
			|| commandLine[position - 1] == L'\t'
			|| commandLine[position - 1] == L'"';
		const std::size_t after = position + flag.size();
		const bool validAfter = after == commandLine.size()
			|| commandLine[after] == L' '
			|| commandLine[after] == L'\t'
			|| commandLine[after] == L'"';
		if (validBefore && validAfter) return true;
		position = commandLine.find(flag, position + 1);
	}
	return false;
}

enum class ContentReloadError
{
	None,
	StageCatalog,
	GameplayCatalog,
	GameplayResolution,
	DifficultyCurve,
	MissingRunStage,
	MissingOrb,
	MissingRelic
};

struct ContentReloadResult
{
	ContentReloadError error = ContentReloadError::None;
	std::size_t errorLine = 0;
	ContentLoadResult content;
	GameplayCatalogLoadResult gameplay;
	DifficultyCurveAnalysis difficulty;
	std::string incompatibleId;

	bool IsReady() const noexcept { return error == ContentReloadError::None; }
};

ContentReloadResult PrepareContentReload(
	const std::filesystem::path& stageCatalogPath,
	const std::filesystem::path& gameplayCatalogPath,
	const std::vector<std::string>& requiredStageIds,
	const PlayerLoadoutSnapshot& activeLoadout);
