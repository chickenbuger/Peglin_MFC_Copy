#pragma once

#include "GameOptions.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct StageRecord
{
	std::string stageId;
	GameDifficulty difficulty = GameDifficulty::Normal;
	int highScore = 0;
	int bestCombo = 0;
	int clearCount = 0;
};

struct PerformanceRecord
{
	std::string stageId;
	GameDifficulty difficulty = GameDifficulty::Normal;
	std::string orbId;
	int attemptCount = 0;
	int clearCount = 0;
	long long totalScore = 0;
	int highScore = 0;
	int bestCombo = 0;

	double AverageScore() const noexcept
	{
		return attemptCount <= 0
			? 0.0
			: static_cast<double>(totalScore) / static_cast<double>(attemptCount);
	}
};

class GameRecordBook
{
public:
	StageRecord Get(std::string_view stageId, GameDifficulty difficulty) const;
	bool ApplyResult(
		std::string_view stageId,
		GameDifficulty difficulty,
		int score,
		int combo,
		bool cleared);
	PerformanceRecord GetPerformance(
		std::string_view stageId,
		GameDifficulty difficulty,
		std::string_view orbId) const;
	bool ApplyPerformanceResult(
		std::string_view stageId,
		GameDifficulty difficulty,
		std::string_view orbId,
		int score,
		int combo,
		bool cleared);
	const std::vector<StageRecord>& GetAll() const noexcept { return _records; }
	const std::vector<PerformanceRecord>& GetAllPerformance() const noexcept { return _performanceRecords; }

private:
	friend class GameRecordStore;
	std::vector<StageRecord> _records;
	std::vector<PerformanceRecord> _performanceRecords;
};

enum class RecordLoadState
{
	Loaded,
	Recovered,
	Migrated,
	Missing,
	Invalid,
	IoError
};

struct RecordLoadResult
{
	GameRecordBook records;
	RecordLoadState state = RecordLoadState::Missing;
	std::string message;
};

class GameRecordStore
{
public:
	explicit GameRecordStore(std::filesystem::path filePath);

	RecordLoadResult Load() const noexcept;
	RecordLoadResult LoadWithRecovery() const noexcept;
	bool Save(const GameRecordBook& records, std::string* errorMessage = nullptr) const noexcept;
	bool Reset(std::string* errorMessage = nullptr) const noexcept;
	const std::filesystem::path& GetFilePath() const noexcept { return _filePath; }
	std::filesystem::path GetBackupPath() const;

private:
	std::filesystem::path _filePath;
};

std::filesystem::path GetDefaultGameRecordPath() noexcept;
