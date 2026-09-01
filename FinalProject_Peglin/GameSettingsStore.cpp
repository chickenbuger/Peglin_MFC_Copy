#include "pch.h"
#include "GameSettingsStore.h"

#include <fstream>
#include <charconv>
#include <map>
#include <system_error>
#include <utility>

namespace
{
	constexpr std::string_view SETTINGS_VERSION_KEY = "peglin_settings_version";
	constexpr std::string_view SETTINGS_VERSION = "3";

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

	std::string PegColorValue(PegColorMode colorMode)
	{
		return colorMode == PegColorMode::HighContrast
			? "high_contrast"
			: "standard";
	}

	std::string LanguageValue(UiLanguage language)
	{
		return language == UiLanguage::English ? "en-US" : "ko-KR";
	}

	std::string GamepadFireValue(GamepadFireBinding binding)
	{
		return binding == GamepadFireBinding::RightTrigger ? "right_trigger" : "south_button";
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

	bool ParseSound(std::string_view value, bool& soundEnabled) noexcept
	{
		if (value == "1")
		{
			soundEnabled = true;
			return true;
		}
		if (value == "0")
		{
			soundEnabled = false;
			return true;
		}

		return false;
	}

	bool ParseVolume(std::string_view value, int& volume) noexcept
	{
		if (value.empty()) return false;
		int parsed = 0;
		const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
		if (result.ec != std::errc{} || result.ptr != value.data() + value.size()
			|| parsed < 0 || parsed > 100)
		{
			return false;
		}
		volume = parsed;
		return true;
	}

	bool ParseIntegerRange(std::string_view value, int minimum, int maximum, int& parsedValue) noexcept
	{
		if (value.empty()) return false;
		int parsed = 0;
		const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
		if (result.ec != std::errc{} || result.ptr != value.data() + value.size()
			|| parsed < minimum || parsed > maximum)
		{
			return false;
		}
		parsedValue = parsed;
		return true;
	}

	bool ParseGamepadFire(std::string_view value, GamepadFireBinding& binding) noexcept
	{
		if (value == "south_button")
		{
			binding = GamepadFireBinding::SouthButton;
			return true;
		}
		if (value == "right_trigger")
		{
			binding = GamepadFireBinding::RightTrigger;
			return true;
		}
		return false;
	}

	bool ParsePegColor(std::string_view value, PegColorMode& colorMode) noexcept
	{
		if (value == "standard")
		{
			colorMode = PegColorMode::Standard;
			return true;
		}
		if (value == "high_contrast")
		{
			colorMode = PegColorMode::HighContrast;
			return true;
		}

		return false;
	}

	bool ParseLanguage(std::string_view value, UiLanguage& language) noexcept
	{
		if (value == "ko-KR")
		{
			language = UiLanguage::Korean;
			return true;
		}
		if (value == "en-US")
		{
			language = UiLanguage::English;
			return true;
		}
		return false;
	}

	bool ParseLegacyDifficulty(std::string_view value, GameDifficulty& difficulty) noexcept
	{
		if (value == "0")
		{
			difficulty = GameDifficulty::Easy;
			return true;
		}
		if (value == "1")
		{
			difficulty = GameDifficulty::Normal;
			return true;
		}
		if (value == "2")
		{
			difficulty = GameDifficulty::Hard;
			return true;
		}
		return false;
	}

	bool ParseLegacySound(std::string_view value, bool& soundEnabled) noexcept
	{
		if (value == "true")
		{
			soundEnabled = true;
			return true;
		}
		if (value == "false")
		{
			soundEnabled = false;
			return true;
		}
		return false;
	}

	bool ParseLegacyPegColor(std::string_view value, PegColorMode& colorMode) noexcept
	{
		if (value == "0")
		{
			colorMode = PegColorMode::Standard;
			return true;
		}
		if (value == "1")
		{
			colorMode = PegColorMode::HighContrast;
			return true;
		}
		return false;
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

GameSettingsStore::GameSettingsStore(std::filesystem::path filePath)
	: _filePath(std::move(filePath))
{
}

SettingsLoadResult GameSettingsStore::Load() const noexcept
{
	SettingsLoadResult result;
	try
	{
		std::error_code existsError;
		if (!std::filesystem::exists(_filePath, existsError))
		{
			result.state = existsError ? SettingsLoadState::IoError : SettingsLoadState::Missing;
			result.message = existsError ? existsError.message() : "settings file is missing";
			return result;
		}

		std::ifstream stream(_filePath);
		if (!stream)
		{
			result.state = SettingsLoadState::IoError;
			result.message = "settings file could not be opened";
			return result;
		}

		std::map<std::string, std::string> values;
		std::string line;
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

			const std::size_t separator = line.find('=');
			if (separator == std::string::npos || separator == 0 || separator + 1 >= line.size())
			{
				result.state = SettingsLoadState::Invalid;
				result.message = "settings line is malformed";
				return result;
			}

			const auto [position, inserted] = values.emplace(
				line.substr(0, separator),
				line.substr(separator + 1));
			UNREFERENCED_PARAMETER(position);
			if (!inserted)
			{
				result.state = SettingsLoadState::Invalid;
				result.message = "settings key is duplicated";
				return result;
			}
		}

		const auto version = values.find(std::string(SETTINGS_VERSION_KEY));
		const auto difficulty = values.find("difficulty");
		const auto sound = values.find("sound_enabled");
		const auto effectsVolume = values.find("effects_volume");
		const auto musicVolume = values.find("music_volume");
		const auto gameplayInfo = values.find("show_gameplay_info");
		const auto color = values.find("peg_color_mode");
		const auto language = values.find("language");
		const auto gamepadDeadzone = values.find("gamepad_deadzone_percent");
		const auto gamepadSensitivity = values.find("gamepad_sensitivity_percent");
		const auto gamepadFire = values.find("gamepad_fire_binding");
		if (version == values.end()
			|| difficulty == values.end()
			|| sound == values.end()
			|| color == values.end())
		{
			result.options = GameOptions{};
			result.state = SettingsLoadState::Invalid;
			result.message = "settings version or value is invalid";
			return result;
		}

		if (version->second == SETTINGS_VERSION)
		{
			if (!ParseDifficulty(difficulty->second, result.options.difficulty)
				|| !ParseSound(sound->second, result.options.soundEnabled)
				|| effectsVolume == values.end()
				|| musicVolume == values.end()
				|| !ParseVolume(effectsVolume->second, result.options.effectsVolume)
				|| !ParseVolume(musicVolume->second, result.options.musicVolume)
				|| (gameplayInfo != values.end()
					&& !ParseSound(gameplayInfo->second, result.options.showGameplayInfo))
				|| !ParsePegColor(color->second, result.options.pegColorMode)
				|| (language != values.end()
					&& !ParseLanguage(language->second, result.options.language))
				|| gamepadDeadzone == values.end()
				|| !ParseIntegerRange(gamepadDeadzone->second, 5, 60, result.options.gamepadDeadzonePercent)
				|| gamepadSensitivity == values.end()
				|| !ParseIntegerRange(gamepadSensitivity->second, 50, 200, result.options.gamepadSensitivityPercent)
				|| gamepadFire == values.end()
				|| !ParseGamepadFire(gamepadFire->second, result.options.gamepadFireBinding))
			{
				result.options = GameOptions{};
				result.state = SettingsLoadState::Invalid;
				result.message = "settings value is invalid";
				return result;
			}
			result.state = SettingsLoadState::Loaded;
			result.message.clear();
			return result;
		}

		if (version->second == "2"
			&& ParseDifficulty(difficulty->second, result.options.difficulty)
			&& ParseSound(sound->second, result.options.soundEnabled)
			&& effectsVolume != values.end()
			&& musicVolume != values.end()
			&& ParseVolume(effectsVolume->second, result.options.effectsVolume)
			&& ParseVolume(musicVolume->second, result.options.musicVolume)
			&& (gameplayInfo == values.end()
				|| ParseSound(gameplayInfo->second, result.options.showGameplayInfo))
			&& ParsePegColor(color->second, result.options.pegColorMode)
			&& (language == values.end()
				|| ParseLanguage(language->second, result.options.language)))
		{
			result.state = SettingsLoadState::Migrated;
			result.message = "settings migrated from version 2";
			return result;
		}

		if (version->second == "1"
			&& ParseDifficulty(difficulty->second, result.options.difficulty)
			&& ParseSound(sound->second, result.options.soundEnabled)
			&& (gameplayInfo == values.end()
				|| ParseSound(gameplayInfo->second, result.options.showGameplayInfo))
			&& ParsePegColor(color->second, result.options.pegColorMode)
			&& (language == values.end()
				|| ParseLanguage(language->second, result.options.language)))
		{
			result.state = SettingsLoadState::Migrated;
			result.message = "settings migrated from version 1";
			return result;
		}

		if (version->second == "0"
			&& ParseLegacyDifficulty(difficulty->second, result.options.difficulty)
			&& ParseLegacySound(sound->second, result.options.soundEnabled)
			&& ParseLegacyPegColor(color->second, result.options.pegColorMode))
		{
			result.state = SettingsLoadState::Migrated;
			result.message = "legacy settings migrated from version 0";
			return result;
		}

		result.options = GameOptions{};
		result.state = SettingsLoadState::Invalid;
		result.message = "settings version or legacy value is invalid";
		return result;
	}
	catch (const std::exception& exception)
	{
		result.options = GameOptions{};
		result.state = SettingsLoadState::IoError;
		result.message = exception.what();
		return result;
	}
}

std::filesystem::path GameSettingsStore::GetBackupPath() const
{
	std::filesystem::path backup = _filePath;
	backup += L".bak";
	return backup;
}

SettingsLoadResult GameSettingsStore::LoadWithRecovery() const noexcept
{
	SettingsLoadResult primary = Load();
	if (primary.state != SettingsLoadState::Invalid
		&& primary.state != SettingsLoadState::IoError)
	{
		return primary;
	}
	const std::filesystem::path backupPath = GetBackupPath();
	GameSettingsStore backupStore(backupPath);
	SettingsLoadResult backup = backupStore.Load();
	if (backup.state != SettingsLoadState::Loaded
		&& backup.state != SettingsLoadState::Migrated)
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
		SettingsLoadResult recovered = Load();
		if (recovered.state == SettingsLoadState::Loaded
			|| recovered.state == SettingsLoadState::Migrated)
		{
			recovered.state = SettingsLoadState::Recovered;
			recovered.message = "settings restored from backup";
			return recovered;
		}
	}
	catch (...)
	{
	}
	return primary;
}

bool GameSettingsStore::Save(const GameOptions& options, std::string* errorMessage) const noexcept
{
	try
	{
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

		std::filesystem::path temporaryPath = _filePath;
		temporaryPath += L".tmp";
		std::ofstream stream(temporaryPath, std::ios::trunc);
		if (!stream)
		{
			if (errorMessage != nullptr)
			{
				*errorMessage = "temporary settings file could not be opened";
			}
			return false;
		}

		stream
			<< SETTINGS_VERSION_KEY << '=' << SETTINGS_VERSION << '\n'
			<< "difficulty=" << DifficultyValue(options.difficulty) << '\n'
			<< "sound_enabled=" << (options.soundEnabled ? '1' : '0') << '\n'
			<< "effects_volume=" << std::clamp(options.effectsVolume, 0, 100) << '\n'
			<< "music_volume=" << std::clamp(options.musicVolume, 0, 100) << '\n'
			<< "show_gameplay_info=" << (options.showGameplayInfo ? '1' : '0') << '\n'
			<< "peg_color_mode=" << PegColorValue(options.pegColorMode) << '\n'
			<< "language=" << LanguageValue(options.language) << '\n'
			<< "gamepad_deadzone_percent=" << std::clamp(options.gamepadDeadzonePercent, 5, 60) << '\n'
			<< "gamepad_sensitivity_percent=" << std::clamp(options.gamepadSensitivityPercent, 50, 200) << '\n'
			<< "gamepad_fire_binding=" << GamepadFireValue(options.gamepadFireBinding) << '\n';
		stream.flush();
		if (!stream)
		{
			stream.close();
			std::error_code cleanupError;
			std::filesystem::remove(temporaryPath, cleanupError);
			if (errorMessage != nullptr)
			{
				*errorMessage = "settings file could not be written";
			}
			return false;
		}
		stream.close();

		std::error_code existsError;
		if (std::filesystem::exists(_filePath, existsError))
		{
			const SettingsLoadResult current = Load();
			if (current.state == SettingsLoadState::Loaded
				|| current.state == SettingsLoadState::Migrated)
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
			if (errorMessage != nullptr)
			{
				*errorMessage = "settings file could not be replaced";
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

bool GameSettingsStore::Reset(std::string* errorMessage) const noexcept
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

std::filesystem::path GetDefaultGameSettingsPath() noexcept
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
				return std::filesystem::path(value) / L"PeglinMFC" / L"settings.v1.ini";
			}
		}
#endif

		std::error_code tempError;
		const std::filesystem::path temporary = std::filesystem::temp_directory_path(tempError);
		if (!tempError)
		{
			return temporary / "PeglinMFC" / "settings.v1.ini";
		}
	}
	catch (...)
	{
	}

	return std::filesystem::path("settings.v1.ini");
}
