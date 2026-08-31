#include "pch.h"
#include "Localization.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <utility>

namespace
{
	constexpr std::size_t MAX_LOCALIZATION_FILE_BYTES = 64 * 1024;

	using StringMap = std::unordered_map<std::string, std::string>;

	StringMap KoreanStrings()
	{
		return {
			{ "screen.stage_selection", "스테이지 선택" },
			{ "screen.loadout", "오브와 유물" },
			{ "screen.options", "옵션" },
			{ "screen.reward", "스테이지 보상" },
			{ "screen.run_complete", "런 완료" },
			{ "screen.run_failed", "런 실패" },
			{ "label.run_rules", "런 규칙" },
			{ "label.current", "현재" },
			{ "label.next", "다음" },
			{ "option.difficulty", "난이도" },
			{ "option.sound", "사운드" },
			{ "option.peg_color", "페그 구분" },
			{ "option.language", "언어" },
			{ "value.on", "켜짐" },
			{ "value.off", "꺼짐" },
			{ "difficulty.easy", "쉬움" },
			{ "difficulty.normal", "보통" },
			{ "difficulty.hard", "어려움" },
			{ "peg_color.standard", "표준 색상" },
			{ "peg_color.high_contrast", "고대비 + 모양" },
			{ "language.korean", "한국어" },
			{ "language.english", "English" },
			{ "hint.start", "현재 노드 시작 · ENTER" },
			{ "hint.loadout", "[L] 장비" },
			{ "hint.options", "[O] 옵션" },
			{ "hint.back", "[B] 또는 ESC로 돌아가기" },
			{ "hint.retry", "다시 도전 · R" },
			{ "hint.new_run", "새 런 · S" },
			{ "notice.auto_save", "변경 내용 자동 저장" },
			{ "notice.settings_save_failed", "설정 저장 실패 · 현재 실행은 계속됩니다" },
			{ "notice.external_fallback", "외부 문자열 누락·손상 · 내장 문구로 복구" }
		};
	}

	StringMap EnglishStrings()
	{
		return {
			{ "screen.stage_selection", "STAGE SELECT" },
			{ "screen.loadout", "ORB & RELIC" },
			{ "screen.options", "OPTIONS" },
			{ "screen.reward", "STAGE REWARD" },
			{ "screen.run_complete", "RUN COMPLETE" },
			{ "screen.run_failed", "RUN FAILED" },
			{ "label.run_rules", "RUN RULES" },
			{ "label.current", "CURRENT" },
			{ "label.next", "NEXT" },
			{ "option.difficulty", "DIFFICULTY" },
			{ "option.sound", "SOUND" },
			{ "option.peg_color", "PEG COLORS" },
			{ "option.language", "LANGUAGE" },
			{ "value.on", "ON" },
			{ "value.off", "OFF" },
			{ "difficulty.easy", "EASY" },
			{ "difficulty.normal", "NORMAL" },
			{ "difficulty.hard", "HARD" },
			{ "peg_color.standard", "STANDARD COLORS" },
			{ "peg_color.high_contrast", "HIGH CONTRAST + SHAPES" },
			{ "language.korean", "한국어" },
			{ "language.english", "English" },
			{ "hint.start", "START CURRENT NODE · ENTER" },
			{ "hint.loadout", "[L] LOADOUT" },
			{ "hint.options", "[O] OPTIONS" },
			{ "hint.back", "[B] OR ESC TO GO BACK" },
			{ "hint.retry", "RETRY · R" },
			{ "hint.new_run", "NEW RUN · S" },
			{ "notice.auto_save", "CHANGES SAVED AUTOMATICALLY" },
			{ "notice.settings_save_failed", "SAVE FAILED · CURRENT SESSION CONTINUES" },
			{ "notice.external_fallback", "EXTERNAL TEXT MISSING OR INVALID · BUILT-IN TEXT ACTIVE" }
		};
	}

	LocalizationLoadResult Fallback(
		UiLanguage language,
		LocalizationLoadError error,
		std::size_t line = 0)
	{
		LocalizationLoadResult result;
		result.error = error;
		result.errorLine = line;
		result.catalog = CreateBuiltInLocalizationCatalog(language);
		result.fallbackKeyCount = result.catalog.Size();
		return result;
	}

	std::string_view Trim(std::string_view text) noexcept
	{
		while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r'))
		{
			text.remove_prefix(1);
		}
		while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
		{
			text.remove_suffix(1);
		}
		return text;
	}

	bool IsValidUtf8(std::string_view text) noexcept
	{
		for (std::size_t index = 0; index < text.size();)
		{
			const unsigned char lead = static_cast<unsigned char>(text[index]);
			if (lead == 0) return false;
			if (lead < 0x80)
			{
				if (lead < 0x20 && lead != '\n' && lead != '\r' && lead != '\t') return false;
				++index;
				continue;
			}
			std::size_t length = 0;
			std::uint32_t codePoint = 0;
			if (lead >= 0xC2 && lead <= 0xDF)
			{
				length = 2;
				codePoint = lead & 0x1Fu;
			}
			else if (lead >= 0xE0 && lead <= 0xEF)
			{
				length = 3;
				codePoint = lead & 0x0Fu;
			}
			else if (lead >= 0xF0 && lead <= 0xF4)
			{
				length = 4;
				codePoint = lead & 0x07u;
			}
			else return false;
			if (index + length > text.size()) return false;
			for (std::size_t offset = 1; offset < length; ++offset)
			{
				const unsigned char continuation = static_cast<unsigned char>(text[index + offset]);
				if ((continuation & 0xC0u) != 0x80u) return false;
				codePoint = (codePoint << 6) | (continuation & 0x3Fu);
			}
			const bool overlong = (length == 3 && codePoint < 0x800u)
				|| (length == 4 && codePoint < 0x10000u);
			if (overlong
				|| (codePoint >= 0xD800u && codePoint <= 0xDFFFu)
				|| codePoint > 0x10FFFFu) return false;
			index += length;
		}
		return true;
	}

	bool IsSafeKey(std::string_view key) noexcept
	{
		if (key.empty() || key.size() > 64) return false;
		for (const char character : key)
		{
			const bool lower = character >= 'a' && character <= 'z';
			const bool digit = character >= '0' && character <= '9';
			if (!lower && !digit && character != '.' && character != '_') return false;
		}
		return true;
	}

	bool DecodeValue(std::string_view encoded, std::string& decoded)
	{
		if (encoded.empty() || encoded.size() > 512) return false;
		decoded.clear();
		decoded.reserve(encoded.size());
		for (std::size_t index = 0; index < encoded.size(); ++index)
		{
			if (encoded[index] != '\\')
			{
				decoded.push_back(encoded[index]);
				continue;
			}
			if (++index >= encoded.size()) return false;
			if (encoded[index] == 'n') decoded.push_back('\n');
			else if (encoded[index] == '\\') decoded.push_back('\\');
			else return false;
		}
		return !decoded.empty();
	}
}

std::string_view LocalizationCatalog::Get(std::string_view key) const noexcept
{
	const auto found = _strings.find(std::string(key));
	return found == _strings.end() ? key : std::string_view(found->second);
}

std::string_view LocaleName(UiLanguage language) noexcept
{
	return language == UiLanguage::English ? "en-US" : "ko-KR";
}

LocalizationCatalog CreateBuiltInLocalizationCatalog(UiLanguage language)
{
	LocalizationCatalog catalog;
	catalog._language = language;
	catalog._strings = language == UiLanguage::English
		? EnglishStrings()
		: KoreanStrings();
	return catalog;
}

LocalizationLoadResult LoadLocalizationCatalog(
	const std::filesystem::path& path,
	UiLanguage expectedLanguage)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		std::error_code error;
		const bool exists = std::filesystem::exists(path, error);
		return Fallback(expectedLanguage, !error && !exists
			? LocalizationLoadError::MissingFile
			: LocalizationLoadError::IoFailure);
	}
	input.seekg(0, std::ios::end);
	const std::streamoff byteCount = input.tellg();
	if (byteCount < 0) return Fallback(expectedLanguage, LocalizationLoadError::IoFailure);
	if (static_cast<std::uintmax_t>(byteCount) > MAX_LOCALIZATION_FILE_BYTES)
	{
		return Fallback(expectedLanguage, LocalizationLoadError::FileTooLarge);
	}
	input.seekg(0, std::ios::beg);
	std::string content{
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>() };
	if (!input.good() && !input.eof()) return Fallback(expectedLanguage, LocalizationLoadError::IoFailure);
	if (!IsValidUtf8(content)) return Fallback(expectedLanguage, LocalizationLoadError::InvalidEncoding);

	LocalizationCatalog catalog = CreateBuiltInLocalizationCatalog(expectedLanguage);
	std::unordered_map<std::string, std::string> externalStrings;
	bool versionSeen = false;
	bool localeSeen = false;
	std::size_t lineNumber = 0;
	std::string_view remaining(content);
	while (!remaining.empty())
	{
		++lineNumber;
		const std::size_t newline = remaining.find('\n');
		const std::string_view line = Trim(remaining.substr(0, newline));
		if (newline == std::string_view::npos) remaining = {};
		else remaining.remove_prefix(newline + 1);
		if (line.empty() || line.front() == '#') continue;

		const std::size_t equals = line.find('=');
		if (equals == std::string_view::npos)
		{
			return Fallback(expectedLanguage, LocalizationLoadError::InvalidValue, lineNumber);
		}
		const std::string_view key = Trim(line.substr(0, equals));
		const std::string_view value = Trim(line.substr(equals + 1));
		if (key == "version")
		{
			if (versionSeen) return Fallback(expectedLanguage, LocalizationLoadError::DuplicateKey, lineNumber);
			if (value != "1") return Fallback(expectedLanguage, LocalizationLoadError::UnsupportedVersion, lineNumber);
			versionSeen = true;
			continue;
		}
		if (key == "locale")
		{
			if (localeSeen) return Fallback(expectedLanguage, LocalizationLoadError::DuplicateKey, lineNumber);
			if (value != LocaleName(expectedLanguage))
			{
				return Fallback(expectedLanguage, LocalizationLoadError::LocaleMismatch, lineNumber);
			}
			localeSeen = true;
			continue;
		}
		if (!IsSafeKey(key) || !catalog._strings.contains(std::string(key)))
		{
			return Fallback(expectedLanguage, LocalizationLoadError::UnknownKey, lineNumber);
		}
		std::string decoded;
		if (!DecodeValue(value, decoded))
		{
			return Fallback(expectedLanguage, LocalizationLoadError::InvalidValue, lineNumber);
		}
		if (!externalStrings.emplace(std::string(key), std::move(decoded)).second)
		{
			return Fallback(expectedLanguage, LocalizationLoadError::DuplicateKey, lineNumber);
		}
	}

	if (!versionSeen || !localeSeen || externalStrings.empty())
	{
		return Fallback(expectedLanguage, LocalizationLoadError::MissingField, lineNumber);
	}
	for (auto& [key, value] : externalStrings)
	{
		catalog._strings[key] = std::move(value);
	}

	LocalizationLoadResult result;
	result.catalog = std::move(catalog);
	result.fallbackKeyCount = result.catalog.Size() - externalStrings.size();
	result.state = result.fallbackKeyCount == 0
		? LocalizationLoadState::External
		: LocalizationLoadState::ExternalWithFallback;
	return result;
}
