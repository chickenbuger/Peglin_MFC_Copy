#include "pch.h"
#include "RunCheckpointStore.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <system_error>
#include <utility>

namespace
{
	constexpr std::string_view CHECKPOINT_VERSION_KEY = "peglin_run_version";
	constexpr std::string_view CHECKPOINT_VERSION = "1";
	constexpr std::uintmax_t MAX_CHECKPOINT_BYTES = 64U * 1024U;
	constexpr std::size_t MAX_CHECKPOINT_LINES = 256;

	bool ReplaceFile(const std::filesystem::path& temporaryPath, const std::filesystem::path& finalPath)
	{
		return ::MoveFileExW(
			temporaryPath.c_str(),
			finalPath.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
	}

	bool IsSafeId(std::string_view id) noexcept
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

	template <typename Integer>
	bool ParseInteger(std::string_view text, Integer minimum, Integer maximum, Integer& value) noexcept
	{
		if (text.empty()) return false;
		Integer parsed{};
		const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
		if (result.ec != std::errc{}
			|| result.ptr != text.data() + text.size()
			|| parsed < minimum
			|| parsed > maximum)
		{
			return false;
		}
		value = parsed;
		return true;
	}

	bool ParseFloat(std::string_view text, float minimum, float maximum, float& value) noexcept
	{
		if (text.empty()) return false;
		float parsed = 0.0f;
		const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
		if (result.ec != std::errc{}
			|| result.ptr != text.data() + text.size()
			|| !std::isfinite(parsed)
			|| parsed < minimum
			|| parsed > maximum)
		{
			return false;
		}
		value = parsed;
		return true;
	}

	std::string StatusValue(RunStatus status)
	{
		switch (status)
		{
		case RunStatus::StageReady: return "stage_ready";
		case RunStatus::RewardSelection: return "reward_selection";
		case RunStatus::StageChoice: return "stage_choice";
		case RunStatus::Complete: return "complete";
		case RunStatus::Defeated: return "defeated";
		case RunStatus::NotStarted: break;
		}
		return "not_started";
	}

	bool ParseStatus(std::string_view text, RunStatus& status) noexcept
	{
		if (text == "stage_ready") status = RunStatus::StageReady;
		else if (text == "reward_selection") status = RunStatus::RewardSelection;
		else if (text == "stage_choice") status = RunStatus::StageChoice;
		else if (text == "complete") status = RunStatus::Complete;
		else if (text == "defeated") status = RunStatus::Defeated;
		else return false;
		return true;
	}

	std::string DifficultyValue(GameDifficulty difficulty)
	{
		switch (difficulty)
		{
		case GameDifficulty::Easy: return "easy";
		case GameDifficulty::Normal: return "normal";
		case GameDifficulty::Hard: return "hard";
		}
		return "normal";
	}

	bool ParseDifficulty(std::string_view text, GameDifficulty& difficulty) noexcept
	{
		if (text == "easy") difficulty = GameDifficulty::Easy;
		else if (text == "normal") difficulty = GameDifficulty::Normal;
		else if (text == "hard") difficulty = GameDifficulty::Hard;
		else return false;
		return true;
	}

	bool ParseLayer(std::string_view text, std::vector<std::string>& layer)
	{
		layer.clear();
		std::size_t start = 0;
		while (start <= text.size())
		{
			const std::size_t separator = text.find(',', start);
			const std::string_view id = text.substr(
				start,
				separator == std::string_view::npos ? text.size() - start : separator - start);
			if (!IsSafeId(id) || layer.size() >= 2)
			{
				return false;
			}
			layer.emplace_back(id);
			if (separator == std::string_view::npos) break;
			start = separator + 1;
		}
		return !layer.empty();
	}

	bool ValidateCheckpoint(const RunCheckpoint& checkpoint) noexcept
	{
		if (checkpoint.difficulty != GameDifficulty::Easy
			&& checkpoint.difficulty != GameDifficulty::Normal
			&& checkpoint.difficulty != GameDifficulty::Hard)
		{
			return false;
		}
		if (!std::isfinite(checkpoint.playerHealth)
			|| checkpoint.playerHealth < 0.0f
			|| checkpoint.playerHealth > 10000.0f)
		{
			return false;
		}
		AdventureRun run;
		if (!run.RestoreSnapshot(checkpoint.run))
		{
			return false;
		}
		PlayerLoadout loadout;
		if (!loadout.RestorePersistentSnapshot(checkpoint.loadout))
		{
			return false;
		}
		const bool anyShopPurchase = std::any_of(
			checkpoint.shopPurchased.begin(),
			checkpoint.shopPurchased.end(),
			[](bool purchased) { return purchased; });
		return !anyShopPurchase
			|| (checkpoint.run.status == RunStatus::StageReady
				&& IsRunShopStage(checkpoint.run.currentStageId));
	}
}

RunCheckpointStore::RunCheckpointStore(std::filesystem::path filePath)
	: _filePath(std::move(filePath))
{
}

RunCheckpointLoadResult RunCheckpointStore::Load() const noexcept
{
	RunCheckpointLoadResult result;
	try
	{
		std::error_code existsError;
		if (!std::filesystem::exists(_filePath, existsError))
		{
			result.state = existsError
				? RunCheckpointLoadState::IoError
				: RunCheckpointLoadState::Missing;
			result.message = existsError ? existsError.message() : "run checkpoint is missing";
			return result;
		}
		std::error_code sizeError;
		const std::uintmax_t fileSize = std::filesystem::file_size(_filePath, sizeError);
		if (sizeError || fileSize > MAX_CHECKPOINT_BYTES)
		{
			result.state = sizeError
				? RunCheckpointLoadState::IoError
				: RunCheckpointLoadState::Invalid;
			result.message = sizeError ? sizeError.message() : "run checkpoint is too large";
			return result;
		}

		std::ifstream stream(_filePath);
		if (!stream)
		{
			result.state = RunCheckpointLoadState::IoError;
			result.message = "run checkpoint could not be opened";
			return result;
		}

		std::map<std::string, std::string> values;
		std::vector<std::string> layers;
		std::vector<std::string> clearedStages;
		std::vector<std::string> ownedOrbs;
		std::vector<std::string> relics;
		std::string line;
		std::size_t lineCount = 0;
		while (std::getline(stream, line))
		{
			if (++lineCount > MAX_CHECKPOINT_LINES)
			{
				result.state = RunCheckpointLoadState::Invalid;
				result.message = "run checkpoint has too many lines";
				return result;
			}
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (line.empty()) continue;
			const std::size_t separator = line.find('=');
			if (separator == std::string::npos || separator == 0 || separator + 1 >= line.size())
			{
				result.state = RunCheckpointLoadState::Invalid;
				result.message = "run checkpoint line is malformed";
				return result;
			}
			const std::string key = line.substr(0, separator);
			const std::string value = line.substr(separator + 1);
			if (key == "stage_layer") layers.push_back(value);
			else if (key == "cleared_stage") clearedStages.push_back(value);
			else if (key == "owned_orb") ownedOrbs.push_back(value);
			else if (key == "relic") relics.push_back(value);
			else
			{
				const auto [position, inserted] = values.emplace(key, value);
				UNREFERENCED_PARAMETER(position);
				if (!inserted)
				{
					result.state = RunCheckpointLoadState::Invalid;
					result.message = "run checkpoint key is duplicated";
					return result;
				}
			}
		}

		const auto version = values.find(std::string(CHECKPOINT_VERSION_KEY));
		if (version == values.end())
		{
			result.state = RunCheckpointLoadState::Invalid;
			result.message = "run checkpoint version is missing";
			return result;
		}
		if (version->second != CHECKPOINT_VERSION)
		{
			result.state = RunCheckpointLoadState::Incompatible;
			result.message = "run checkpoint version is not supported";
			return result;
		}

		const std::array<std::string_view, 10> requiredKeys{
			"status", "difficulty", "current_layer", "completed_combats", "gold",
			"player_health", "current_stage", "selected_choice", "shop_purchased",
			"preferred_orb"
		};
		for (const std::string_view key : requiredKeys)
		{
			if (!values.contains(std::string(key)))
			{
				result.state = RunCheckpointLoadState::Invalid;
				result.message = "run checkpoint value is missing";
				return result;
			}
		}
		if (values.size() != requiredKeys.size() + 1
			|| layers.empty()
			|| ownedOrbs.empty())
		{
			result.state = RunCheckpointLoadState::Invalid;
			result.message = "run checkpoint contains unknown or incomplete data";
			return result;
		}

		RunCheckpoint checkpoint;
		if (!ParseStatus(values["status"], checkpoint.run.status)
			|| !ParseDifficulty(values["difficulty"], checkpoint.difficulty)
			|| !ParseInteger<std::size_t>(values["current_layer"], 0, 31, checkpoint.run.currentLayer)
			|| !ParseInteger<std::size_t>(values["completed_combats"], 0, 32, checkpoint.run.completedCombatStages)
			|| !ParseInteger<int>(values["gold"], 0, 999999, checkpoint.run.gold)
			|| !ParseFloat(values["player_health"], 0.0f, 10000.0f, checkpoint.playerHealth)
			|| !IsSafeId(values["current_stage"]))
		{
			result.state = RunCheckpointLoadState::Invalid;
			result.message = "run checkpoint scalar value is invalid";
			return result;
		}
		checkpoint.run.currentStageId = values["current_stage"];

		int selectedChoice = -1;
		if (!ParseInteger<int>(values["selected_choice"], -1, 1, selectedChoice))
		{
			result.state = RunCheckpointLoadState::Invalid;
			result.message = "run checkpoint selection is invalid";
			return result;
		}
		if (selectedChoice >= 0)
		{
			checkpoint.run.selectedStageChoiceIndex = static_cast<std::size_t>(selectedChoice);
		}
		const std::string& shop = values["shop_purchased"];
		if (shop.size() != 3
			|| std::any_of(shop.begin(), shop.end(), [](char value) { return value != '0' && value != '1'; }))
		{
			result.state = RunCheckpointLoadState::Invalid;
			result.message = "run checkpoint shop state is invalid";
			return result;
		}
		for (std::size_t index = 0; index < checkpoint.shopPurchased.size(); ++index)
		{
			checkpoint.shopPurchased[index] = shop[index] == '1';
		}

		for (const std::string& encodedLayer : layers)
		{
			std::vector<std::string> layer;
			if (!ParseLayer(encodedLayer, layer))
			{
				result.state = RunCheckpointLoadState::Invalid;
				result.message = "run checkpoint route is invalid";
				return result;
			}
			checkpoint.run.stageLayers.push_back(std::move(layer));
		}
		for (const std::string& id : clearedStages)
		{
			if (!IsSafeId(id))
			{
				result.state = RunCheckpointLoadState::Invalid;
				result.message = "run checkpoint cleared stage is invalid";
				return result;
			}
			checkpoint.run.clearedStageIds.push_back(id);
		}
		for (const std::string& id : ownedOrbs)
		{
			if (!IsSafeId(id))
			{
				result.state = RunCheckpointLoadState::Invalid;
				result.message = "run checkpoint orb is invalid";
				return result;
			}
			checkpoint.loadout.ownedOrbIds.push_back(id);
		}
		const auto preferred = values.find("preferred_orb");
		if (preferred == values.end() || !IsSafeId(preferred->second))
		{
			result.state = RunCheckpointLoadState::Invalid;
			result.message = "run checkpoint preferred orb is invalid";
			return result;
		}
		checkpoint.loadout.preferredOrbId = preferred->second;
		for (const std::string& id : relics)
		{
			if (!IsSafeId(id))
			{
				result.state = RunCheckpointLoadState::Invalid;
				result.message = "run checkpoint relic is invalid";
				return result;
			}
			checkpoint.loadout.acquiredRelics.push_back(id);
		}

		if (!ValidateCheckpoint(checkpoint))
		{
			result.state = RunCheckpointLoadState::Invalid;
			result.message = "run checkpoint relationships are invalid";
			return result;
		}
		result.checkpoint = std::move(checkpoint);
		result.state = RunCheckpointLoadState::Loaded;
		result.message.clear();
		return result;
	}
	catch (const std::exception& exception)
	{
		result.state = RunCheckpointLoadState::IoError;
		result.message = exception.what();
		return result;
	}
}

std::filesystem::path RunCheckpointStore::GetBackupPath() const
{
	std::filesystem::path backup = _filePath;
	backup += L".bak";
	return backup;
}

RunCheckpointLoadResult RunCheckpointStore::LoadWithRecovery() const noexcept
{
	RunCheckpointLoadResult primary = Load();
	if (primary.state != RunCheckpointLoadState::Invalid
		&& primary.state != RunCheckpointLoadState::IoError)
	{
		return primary;
	}
	const std::filesystem::path backupPath = GetBackupPath();
	RunCheckpointStore backupStore(backupPath);
	RunCheckpointLoadResult backup = backupStore.Load();
	if (backup.state != RunCheckpointLoadState::Loaded)
	{
		return primary;
	}
	try
	{
		std::filesystem::path recoveryPath = _filePath;
		recoveryPath += L".recovery";
		std::error_code copyError;
		std::filesystem::copy_file(
			backupPath,
			recoveryPath,
			std::filesystem::copy_options::overwrite_existing,
			copyError);
		if (copyError || !ReplaceFile(recoveryPath, _filePath))
		{
			std::error_code cleanupError;
			std::filesystem::remove(recoveryPath, cleanupError);
			return primary;
		}
		RunCheckpointLoadResult recovered = Load();
		if (recovered.state == RunCheckpointLoadState::Loaded)
		{
			recovered.state = RunCheckpointLoadState::Recovered;
			recovered.message = "run checkpoint restored from backup";
			return recovered;
		}
	}
	catch (...)
	{
	}
	return primary;
}

bool RunCheckpointStore::Save(
	const RunCheckpoint& checkpoint,
	std::string* errorMessage) const noexcept
{
	try
	{
		if (!ValidateCheckpoint(checkpoint))
		{
			if (errorMessage != nullptr) *errorMessage = "run checkpoint is invalid";
			return false;
		}
		std::error_code directoryError;
		const std::filesystem::path parent = _filePath.parent_path();
		if (!parent.empty())
		{
			std::filesystem::create_directories(parent, directoryError);
		}
		if (directoryError)
		{
			if (errorMessage != nullptr) *errorMessage = directoryError.message();
			return false;
		}

		std::filesystem::path temporaryPath = _filePath;
		temporaryPath += L".tmp";
		std::ofstream stream(temporaryPath, std::ios::trunc);
		if (!stream)
		{
			if (errorMessage != nullptr) *errorMessage = "temporary run checkpoint could not be opened";
			return false;
		}
		stream
			<< CHECKPOINT_VERSION_KEY << '=' << CHECKPOINT_VERSION << '\n'
			<< "status=" << StatusValue(checkpoint.run.status) << '\n'
			<< "difficulty=" << DifficultyValue(checkpoint.difficulty) << '\n'
			<< "current_layer=" << checkpoint.run.currentLayer << '\n'
			<< "completed_combats=" << checkpoint.run.completedCombatStages << '\n'
			<< "gold=" << checkpoint.run.gold << '\n'
			<< "player_health=" << std::setprecision(9) << checkpoint.playerHealth << '\n'
			<< "current_stage=" << checkpoint.run.currentStageId << '\n'
			<< "selected_choice="
			<< (checkpoint.run.selectedStageChoiceIndex.has_value()
				? static_cast<int>(*checkpoint.run.selectedStageChoiceIndex)
				: -1) << '\n'
			<< "shop_purchased="
			<< (checkpoint.shopPurchased[0] ? '1' : '0')
			<< (checkpoint.shopPurchased[1] ? '1' : '0')
			<< (checkpoint.shopPurchased[2] ? '1' : '0') << '\n'
			<< "preferred_orb=" << checkpoint.loadout.preferredOrbId << '\n';
		for (const std::vector<std::string>& layer : checkpoint.run.stageLayers)
		{
			stream << "stage_layer=" << layer.front();
			if (layer.size() == 2) stream << ',' << layer[1];
			stream << '\n';
		}
		for (const std::string& id : checkpoint.run.clearedStageIds)
		{
			stream << "cleared_stage=" << id << '\n';
		}
		for (const std::string& id : checkpoint.loadout.ownedOrbIds)
		{
			stream << "owned_orb=" << id << '\n';
		}
		for (const std::string& id : checkpoint.loadout.acquiredRelics)
		{
			stream << "relic=" << id << '\n';
		}
		stream.flush();
		if (!stream)
		{
			stream.close();
			std::error_code cleanupError;
			std::filesystem::remove(temporaryPath, cleanupError);
			if (errorMessage != nullptr) *errorMessage = "run checkpoint could not be written";
			return false;
		}
		stream.close();

		std::error_code existsError;
		if (std::filesystem::exists(_filePath, existsError))
		{
			if (Load().state == RunCheckpointLoadState::Loaded)
			{
				std::error_code backupError;
				std::filesystem::copy_file(
					_filePath,
					GetBackupPath(),
					std::filesystem::copy_options::overwrite_existing,
					backupError);
				if (backupError)
				{
					std::error_code cleanupError;
					std::filesystem::remove(temporaryPath, cleanupError);
					if (errorMessage != nullptr) *errorMessage = backupError.message();
					return false;
				}
			}
		}
		else if (existsError)
		{
			std::error_code cleanupError;
			std::filesystem::remove(temporaryPath, cleanupError);
			if (errorMessage != nullptr) *errorMessage = existsError.message();
			return false;
		}
		if (!ReplaceFile(temporaryPath, _filePath))
		{
			std::error_code cleanupError;
			std::filesystem::remove(temporaryPath, cleanupError);
			if (errorMessage != nullptr) *errorMessage = "run checkpoint could not be replaced";
			return false;
		}
		if (errorMessage != nullptr) errorMessage->clear();
		return true;
	}
	catch (const std::exception& exception)
	{
		if (errorMessage != nullptr) *errorMessage = exception.what();
		return false;
	}
}

bool RunCheckpointStore::Reset(std::string* errorMessage) const noexcept
{
	try
	{
		std::filesystem::path temporary = _filePath;
		temporary += L".tmp";
		std::filesystem::path recovery = _filePath;
		recovery += L".recovery";
		for (const std::filesystem::path& path : { _filePath, GetBackupPath(), temporary, recovery })
		{
			std::error_code error;
			std::filesystem::remove(path, error);
			if (error)
			{
				if (errorMessage != nullptr) *errorMessage = error.message();
				return false;
			}
		}
		if (errorMessage != nullptr) errorMessage->clear();
		return true;
	}
	catch (const std::exception& exception)
	{
		if (errorMessage != nullptr) *errorMessage = exception.what();
		return false;
	}
}

std::filesystem::path GetDefaultRunCheckpointPath() noexcept
{
	try
	{
		const DWORD requiredLength = ::GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
		if (requiredLength > 1)
		{
			std::wstring value(requiredLength, L'\0');
			const DWORD written = ::GetEnvironmentVariableW(
				L"LOCALAPPDATA",
				value.data(),
				requiredLength);
			if (written > 0 && written < requiredLength)
			{
				value.resize(written);
				return std::filesystem::path(value) / L"PeglinMFC" / L"run.v1.ini";
			}
		}
		std::error_code tempError;
		const std::filesystem::path temporary = std::filesystem::temp_directory_path(tempError);
		if (!tempError)
		{
			return temporary / "PeglinMFC" / "run.v1.ini";
		}
	}
	catch (...)
	{
	}
	return std::filesystem::path("run.v1.ini");
}
