#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

struct ContentReportResult
{
	bool success = false;
	std::size_t stageCount = 0;
	std::size_t pegCount = 0;
	std::size_t movingPegCount = 0;
	std::size_t refreshPegCount = 0;
	std::size_t enemyCount = 0;
	std::size_t orbCount = 0;
	std::size_t relicCount = 0;
	bool difficultyCurvePassed = false;
	std::size_t difficultyIssueCount = 0;
	std::string message;
};

ContentReportResult GenerateContentReport(
	const std::filesystem::path& stageCatalogPath,
	const std::filesystem::path& gameplayCatalogPath,
	const std::filesystem::path& outputPath);
