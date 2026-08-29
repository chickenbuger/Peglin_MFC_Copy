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
	const std::vector<StageRecord>& GetAll() const noexcept { return _records; }

private:
	friend class GameRecordStore;
	std::vector<StageRecord> _records;
};

enum class RecordLoadState
{
	Loaded,
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
	bool Save(const GameRecordBook& records, std::string* errorMessage = nullptr) const noexcept;
	const std::filesystem::path& GetFilePath() const noexcept { return _filePath; }

private:
	std::filesystem::path _filePath;
};

std::filesystem::path GetDefaultGameRecordPath() noexcept;
