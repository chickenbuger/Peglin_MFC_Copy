#include "pch.h"
#include "GameRecordStore.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <set>
#include <system_error>
#include <tuple>
#include <utility>

namespace
{
	constexpr std::string_view RECORDS_VERSION_KEY = "peglin_records_version";
	constexpr std::string_view RECORDS_VERSION = "1";
	constexpr std::size_t MAX_RECORDS = 128;

	bool IsValidStageId(std::string_view stageId) noexcept
	{
		if (stageId.empty() || stageId.size() > 64)
		{
			return false;
		}

		return std::all_of(stageId.begin(), stageId.end(), [](char character)
		{
			return (character >= 'a' && character <= 'z')
				|| (character >= 'A' && character <= 'Z')
				|| (character >= '0' && character <= '9')
				|| character == '-'
				|| character == '_';
		});
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

	bool ParseDifficulty(std::string_view value, GameDifficulty& difficulty) noexcept
	{
		if (value == "easy")
		{
			difficulty = GameDifficulty::Easy;
			return true;
		}
		if (value == "normal")
		{
			difficulty = GameDifficulty::Normal;
			return true;
		}
		if (value == "hard")
		{
			difficulty = GameDifficulty::Hard;
			return true;
		}

		return false;
	}

	bool ParseNonNegativeInt(std::string_view value, int& output) noexcept
	{
		if (value.empty())
		{
			return false;
		}

		int parsed = 0;
		const char* const begin = value.data();
		const char* const end = begin + value.size();
		const auto result = std::from_chars(begin, end, parsed);
		if (result.ec != std::errc{} || result.ptr != end || parsed < 0)
		{
			return false;
		}

		output = parsed;
		return true;
	}

	std::vector<std::string_view> SplitRecord(std::string_view value)
	{
		std::vector<std::string_view> fields;
		std::size_t start = 0;
		while (start <= value.size())
		{
			const std::size_t separator = value.find('|', start);
			if (separator == std::string_view::npos)
			{
				fields.push_back(value.substr(start));
				break;
			}
			fields.push_back(value.substr(start, separator - start));
			start = separator + 1;
		}
		return fields;
	}

	bool ReplaceFile(const std::filesystem::path& temporaryPath, const std::filesystem::path& finalPath)
	{
#ifdef _WIN32
		return ::MoveFileExW(
			temporaryPath.c_str(),
			finalPath.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
#else
		std::error_code error;
		std::filesystem::rename(temporaryPath, finalPath, error);
		return !error;
#endif
	}
}

StageRecord GameRecordBook::Get(std::string_view stageId, GameDifficulty difficulty) const
{
	const auto record = std::find_if(_records.begin(), _records.end(), [stageId, difficulty](const StageRecord& candidate)
	{
		return candidate.stageId == stageId && candidate.difficulty == difficulty;
	});
	return record == _records.end()
		? StageRecord{ std::string(stageId), difficulty, 0, 0, 0 }
		: *record;
}

bool GameRecordBook::ApplyResult(
	std::string_view stageId,
	GameDifficulty difficulty,
	int score,
	int combo,
	bool cleared)
{
	if (!IsValidStageId(stageId) || score < 0 || combo < 0)
	{
		return false;
	}

	auto record = std::find_if(_records.begin(), _records.end(), [stageId, difficulty](const StageRecord& candidate)
	{
		return candidate.stageId == stageId && candidate.difficulty == difficulty;
	});
	if (record == _records.end())
	{
		if (score == 0 && combo == 0 && !cleared)
		{
			return false;
		}
		_records.push_back(StageRecord{ std::string(stageId), difficulty, score, combo, cleared ? 1 : 0 });
		return true;
	}

	bool changed = false;
	if (score > record->highScore)
	{
		record->highScore = score;
		changed = true;
	}
	if (combo > record->bestCombo)
	{
		record->bestCombo = combo;
		changed = true;
	}
	if (cleared && record->clearCount < (std::numeric_limits<int>::max)())
	{
		++record->clearCount;
		changed = true;
	}

	return changed;
}

GameRecordStore::GameRecordStore(std::filesystem::path filePath)
	: _filePath(std::move(filePath))
{
}

RecordLoadResult GameRecordStore::Load() const noexcept
{
	RecordLoadResult result;
	try
	{
		std::error_code existsError;
		if (!std::filesystem::exists(_filePath, existsError))
		{
			result.state = existsError ? RecordLoadState::IoError : RecordLoadState::Missing;
			result.message = existsError ? existsError.message() : "records file is missing";
			return result;
		}

		std::ifstream stream(_filePath);
		if (!stream)
		{
			result.state = RecordLoadState::IoError;
			result.message = "records file could not be opened";
			return result;
		}

		std::string line;
		if (!std::getline(stream, line))
		{
			result.state = RecordLoadState::Invalid;
			result.message = "records file is empty";
			return result;
		}
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		const bool legacyVersion = line == std::string(RECORDS_VERSION_KEY) + "=0";
		if (!legacyVersion && line != std::string(RECORDS_VERSION_KEY) + '=' + std::string(RECORDS_VERSION))
		{
			result.state = RecordLoadState::Invalid;
			result.message = "records version is invalid";
			return result;
		}

		std::set<std::pair<std::string, GameDifficulty>> keys;
		while (std::getline(stream, line))
		{
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}
			if (line.empty())
			{
				continue;
			}
			if (!line.starts_with("record="))
			{
				result = RecordLoadResult{};
				result.state = RecordLoadState::Invalid;
				result.message = "record line is malformed";
				return result;
			}

			const std::vector<std::string_view> fields = SplitRecord(std::string_view(line).substr(7));
			StageRecord record;
			const std::size_t expectedFields = legacyVersion ? 4 : 5;
			if (fields.size() != expectedFields
				|| !IsValidStageId(fields[0])
				|| (!legacyVersion && !ParseDifficulty(fields[1], record.difficulty)))
			{
				result = RecordLoadResult{};
				result.state = RecordLoadState::Invalid;
				result.message = "record value is invalid";
				return result;
			}
			const std::size_t scoreIndex = legacyVersion ? 1 : 2;
			if (!ParseNonNegativeInt(fields[scoreIndex], record.highScore)
				|| !ParseNonNegativeInt(fields[scoreIndex + 1], record.bestCombo)
				|| !ParseNonNegativeInt(fields[scoreIndex + 2], record.clearCount))
			{
				result = RecordLoadResult{};
				result.state = RecordLoadState::Invalid;
				result.message = "record value is invalid";
				return result;
			}
			if (legacyVersion)
			{
				record.difficulty = GameDifficulty::Normal;
			}
			record.stageId = fields[0];
			if (!keys.emplace(record.stageId, record.difficulty).second)
			{
				result = RecordLoadResult{};
				result.state = RecordLoadState::Invalid;
				result.message = "record key is duplicated";
				return result;
			}
			result.records._records.push_back(std::move(record));
			if (result.records._records.size() > MAX_RECORDS)
			{
				result = RecordLoadResult{};
				result.state = RecordLoadState::Invalid;
				result.message = "too many records";
				return result;
			}
		}

		result.state = legacyVersion ? RecordLoadState::Migrated : RecordLoadState::Loaded;
		result.message = legacyVersion ? "legacy records migrated from version 0" : "";
		return result;
	}
	catch (const std::exception& exception)
	{
		result = RecordLoadResult{};
		result.state = RecordLoadState::IoError;
		result.message = exception.what();
		return result;
	}
}

bool GameRecordStore::Save(const GameRecordBook& records, std::string* errorMessage) const noexcept
{
	try
	{
		if (records.GetAll().size() > MAX_RECORDS)
		{
			if (errorMessage != nullptr)
			{
				*errorMessage = "too many records";
			}
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
			if (errorMessage != nullptr)
			{
				*errorMessage = directoryError.message();
			}
			return false;
		}

		std::vector<StageRecord> sorted = records.GetAll();
		std::sort(sorted.begin(), sorted.end(), [](const StageRecord& left, const StageRecord& right)
		{
			return std::tie(left.stageId, left.difficulty) < std::tie(right.stageId, right.difficulty);
		});

		std::filesystem::path temporaryPath = _filePath;
		temporaryPath += L".tmp";
		std::ofstream stream(temporaryPath, std::ios::trunc);
		if (!stream)
		{
			if (errorMessage != nullptr)
			{
				*errorMessage = "temporary records file could not be opened";
			}
			return false;
		}
		stream << RECORDS_VERSION_KEY << '=' << RECORDS_VERSION << '\n';
		for (const StageRecord& record : sorted)
		{
			if (!IsValidStageId(record.stageId)
				|| record.highScore < 0
				|| record.bestCombo < 0
				|| record.clearCount < 0)
			{
				stream.close();
				std::error_code cleanupError;
				std::filesystem::remove(temporaryPath, cleanupError);
				if (errorMessage != nullptr)
				{
					*errorMessage = "record value is invalid";
				}
				return false;
			}
			stream
				<< "record=" << record.stageId << '|'
				<< DifficultyValue(record.difficulty) << '|'
				<< record.highScore << '|'
				<< record.bestCombo << '|'
				<< record.clearCount << '\n';
		}
		stream.flush();
		if (!stream)
		{
			stream.close();
			std::error_code cleanupError;
			std::filesystem::remove(temporaryPath, cleanupError);
			if (errorMessage != nullptr)
			{
				*errorMessage = "records file could not be written";
			}
			return false;
		}
		stream.close();

		if (!ReplaceFile(temporaryPath, _filePath))
		{
			std::error_code cleanupError;
			std::filesystem::remove(temporaryPath, cleanupError);
			if (errorMessage != nullptr)
			{
				*errorMessage = "records file could not be replaced";
			}
			return false;
		}

		if (errorMessage != nullptr)
		{
			errorMessage->clear();
		}
		return true;
	}
	catch (const std::exception& exception)
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = exception.what();
		}
		return false;
	}
}

std::filesystem::path GetDefaultGameRecordPath() noexcept
{
	try
	{
#ifdef _WIN32
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
				return std::filesystem::path(value) / L"PeglinMFC" / L"records.v1.ini";
			}
		}
#endif

		std::error_code tempError;
		const std::filesystem::path temporary = std::filesystem::temp_directory_path(tempError);
		if (!tempError)
		{
			return temporary / "PeglinMFC" / "records.v1.ini";
		}
	}
	catch (...)
	{
	}

	return std::filesystem::path("records.v1.ini");
}
