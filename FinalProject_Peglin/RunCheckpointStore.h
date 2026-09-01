#pragma once

#include "GameOptions.h"
#include "Progression.h"
#include "RunProgression.h"

#include <array>
#include <filesystem>
#include <string>

struct RunCheckpoint
{
	AdventureRunSnapshot run;
	PlayerLoadoutSnapshot loadout;
	GameDifficulty difficulty = GameDifficulty::Normal;
	float playerHealth = 0.0f;
	std::array<bool, 3> shopPurchased{};
};

enum class RunCheckpointLoadState
{
	Loaded,
	Recovered,
	Missing,
	Invalid,
	Incompatible,
	IoError
};

struct RunCheckpointLoadResult
{
	RunCheckpoint checkpoint;
	RunCheckpointLoadState state = RunCheckpointLoadState::Missing;
	std::string message;

	bool IsUsable() const noexcept
	{
		return state == RunCheckpointLoadState::Loaded
			|| state == RunCheckpointLoadState::Recovered;
	}
};

class RunCheckpointStore
{
public:
	explicit RunCheckpointStore(std::filesystem::path filePath);

	RunCheckpointLoadResult Load() const noexcept;
	RunCheckpointLoadResult LoadWithRecovery() const noexcept;
	bool Save(const RunCheckpoint& checkpoint, std::string* errorMessage = nullptr) const noexcept;
	bool Reset(std::string* errorMessage = nullptr) const noexcept;
	const std::filesystem::path& GetFilePath() const noexcept { return _filePath; }
	std::filesystem::path GetBackupPath() const;

private:
	std::filesystem::path _filePath;
};

std::filesystem::path GetDefaultRunCheckpointPath() noexcept;
