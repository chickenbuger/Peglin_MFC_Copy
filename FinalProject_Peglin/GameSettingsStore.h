#pragma once

#include "GameOptions.h"

#include <filesystem>
#include <string>

enum class SettingsLoadState
{
	Loaded,
	Recovered,
	Migrated,
	Missing,
	Invalid,
	IoError
};

struct SettingsLoadResult
{
	GameOptions options;
	SettingsLoadState state = SettingsLoadState::Missing;
	std::string message;
};

class GameSettingsStore
{
public:
	explicit GameSettingsStore(std::filesystem::path filePath);

	SettingsLoadResult Load() const noexcept;
	SettingsLoadResult LoadWithRecovery() const noexcept;
	bool Save(const GameOptions& options, std::string* errorMessage = nullptr) const noexcept;
	bool Reset(std::string* errorMessage = nullptr) const noexcept;
	const std::filesystem::path& GetFilePath() const noexcept { return _filePath; }
	std::filesystem::path GetBackupPath() const;

private:
	std::filesystem::path _filePath;
};

std::filesystem::path GetDefaultGameSettingsPath() noexcept;
