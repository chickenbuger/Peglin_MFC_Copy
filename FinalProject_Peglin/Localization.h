#pragma once

#include "GameOptions.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

enum class LocalizationLoadState
{
	External,
	ExternalWithFallback,
	BuiltInFallback
};

enum class LocalizationLoadError
{
	None,
	MissingFile,
	IoFailure,
	FileTooLarge,
	InvalidEncoding,
	UnsupportedVersion,
	LocaleMismatch,
	UnknownKey,
	DuplicateKey,
	MissingField,
	InvalidValue
};

struct LocalizationLoadResult;

class LocalizationCatalog
{
public:
	std::string_view Get(std::string_view key) const noexcept;
	UiLanguage GetLanguage() const noexcept { return _language; }
	std::size_t Size() const noexcept { return _strings.size(); }

private:
	friend struct LocalizationLoadResult;
	friend LocalizationCatalog CreateBuiltInLocalizationCatalog(UiLanguage language);
	friend LocalizationLoadResult LoadLocalizationCatalog(
		const std::filesystem::path& path,
		UiLanguage expectedLanguage);

	UiLanguage _language = UiLanguage::Korean;
	std::unordered_map<std::string, std::string> _strings;
};

struct LocalizationLoadResult
{
	LocalizationLoadState state = LocalizationLoadState::BuiltInFallback;
	LocalizationLoadError error = LocalizationLoadError::None;
	std::size_t errorLine = 0;
	std::size_t fallbackKeyCount = 0;
	LocalizationCatalog catalog;

	bool UsedExternalContent() const noexcept
	{
		return state != LocalizationLoadState::BuiltInFallback
			&& error == LocalizationLoadError::None;
	}
};

std::string_view LocaleName(UiLanguage language) noexcept;
LocalizationCatalog CreateBuiltInLocalizationCatalog(UiLanguage language);
LocalizationLoadResult LoadLocalizationCatalog(
	const std::filesystem::path& path,
	UiLanguage expectedLanguage);
