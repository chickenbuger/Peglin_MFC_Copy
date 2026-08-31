#include "pch.h"
#include "ContentCatalog.h"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

namespace
{
	constexpr std::size_t MAX_CONTENT_FILE_BYTES = 256 * 1024;
	constexpr std::size_t MAX_CONTENT_STAGES = 32;
	constexpr std::size_t MAX_STAGE_ID_LENGTH = 64;
	constexpr std::size_t MAX_STAGE_NAME_LENGTH = 80;

	struct StageBuilder
	{
		StageDefinition stage;
		std::unordered_set<std::string> scalarKeys;
		std::unordered_set<std::size_t> pegOverrideIndexes;
		bool hasId = false;
		bool hasName = false;
		bool hasLayout = false;
		bool hasPlayerHealth = false;
		bool hasEnemyHealth = false;
		bool hasPlayerDamage = false;
		bool hasEnemySteps = false;
		bool hasEnemyStep = false;
		bool hasRestitution = false;
		bool hasBoss = false;
	};

	ContentLoadResult Fallback(ContentLoadError error, std::size_t line = 0)
	{
		ContentLoadResult result;
		result.error = error;
		result.errorLine = line;
		result.stages = CreateBuiltInContentCatalog();
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

	std::vector<std::string_view> Split(std::string_view text, char delimiter)
	{
		std::vector<std::string_view> values;
		while (true)
		{
			const std::size_t delimiterIndex = text.find(delimiter);
			values.push_back(Trim(text.substr(0, delimiterIndex)));
			if (delimiterIndex == std::string_view::npos)
			{
				break;
			}
			text.remove_prefix(delimiterIndex + 1);
		}
		return values;
	}

	template<typename Number>
	bool ParseNumber(std::string_view text, Number& value) noexcept
	{
		text = Trim(text);
		if (text.empty())
		{
			return false;
		}
		const char* begin = text.data();
		const char* end = begin + text.size();
		const std::from_chars_result parsed = std::from_chars(begin, end, value);
		return parsed.ec == std::errc{} && parsed.ptr == end;
	}

	bool ParseFiniteFloat(std::string_view text, float& value) noexcept
	{
		return ParseNumber(text, value) && std::isfinite(value);
	}

	bool IsSafeStageId(std::string_view id) noexcept
	{
		if (id.empty() || id.size() > MAX_STAGE_ID_LENGTH)
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

	bool IsValidUtf8(std::string_view text) noexcept
	{
		for (std::size_t index = 0; index < text.size();)
		{
			const unsigned char lead = static_cast<unsigned char>(text[index]);
			if (lead == 0)
			{
				return false;
			}
			if (lead < 0x80)
			{
				if (lead < 0x20 && lead != '\n' && lead != '\r' && lead != '\t')
				{
					return false;
				}
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
			else
			{
				return false;
			}
			if (index + length > text.size())
			{
				return false;
			}
			for (std::size_t offset = 1; offset < length; ++offset)
			{
				const unsigned char continuation = static_cast<unsigned char>(text[index + offset]);
				if ((continuation & 0xC0u) != 0x80u)
				{
					return false;
				}
				codePoint = (codePoint << 6) | (continuation & 0x3Fu);
			}
			const bool overlong = (length == 3 && codePoint < 0x800u)
				|| (length == 4 && codePoint < 0x10000u);
			if (overlong
				|| (codePoint >= 0xD800u && codePoint <= 0xDFFFu)
				|| codePoint > 0x10FFFFu)
			{
				return false;
			}
			index += length;
		}
		return true;
	}

	bool ParsePegType(std::string_view text, PegType& type) noexcept
	{
		if (text == "Normal") type = PegType::Normal;
		else if (text == "Critical") type = PegType::Critical;
		else if (text == "Bomb") type = PegType::Bomb;
		else if (text == "Refresh") type = PegType::Refresh;
		else return false;
		return true;
	}

	bool ParseActionType(std::string_view text, EnemyActionType& type) noexcept
	{
		if (text == "Advance") type = EnemyActionType::Advance;
		else if (text == "Strike") type = EnemyActionType::Strike;
		else if (text == "Fortify") type = EnemyActionType::Fortify;
		else return false;
		return true;
	}

	bool ParseEnemyVisual(std::string_view text, EnemyVisualKind& visual) noexcept
	{
		if (text == "CrystalToad") visual = EnemyVisualKind::CrystalToad;
		else if (text == "EmberBat") visual = EnemyVisualKind::EmberBat;
		else if (text == "MossShaman") visual = EnemyVisualKind::MossShaman;
		else return false;
		return true;
	}

	bool HasEveryRequiredField(const StageBuilder& builder) noexcept
	{
		return builder.hasId
			&& builder.hasName
			&& builder.hasLayout
			&& builder.hasPlayerHealth
			&& builder.hasEnemyHealth
			&& builder.hasPlayerDamage
			&& builder.hasEnemySteps
			&& builder.hasEnemyStep
			&& builder.hasRestitution
			&& builder.hasBoss;
	}

	ContentLoadResult ParseContent(std::string_view content)
	{
		if (!IsValidUtf8(content))
		{
			return Fallback(ContentLoadError::InvalidEncoding);
		}

		std::vector<StageDefinition> stages;
		std::unordered_set<std::string> stageIds;
		StageBuilder builder;
		bool insideStage = false;
		bool versionSeen = false;
		std::size_t lineNumber = 0;
		while (!content.empty())
		{
			++lineNumber;
			const std::size_t newline = content.find('\n');
			std::string_view line = Trim(content.substr(0, newline));
			if (newline == std::string_view::npos) content = {};
			else content.remove_prefix(newline + 1);
			if (line.empty() || line.front() == '#')
			{
				continue;
			}

			if (line == "[stage]")
			{
				if (insideStage || !versionSeen || stages.size() >= MAX_CONTENT_STAGES)
				{
					return Fallback(ContentLoadError::UnexpectedSection, lineNumber);
				}
				insideStage = true;
				builder = {};
				continue;
			}
			if (line == "[/stage]")
			{
				if (!insideStage)
				{
					return Fallback(ContentLoadError::UnexpectedSection, lineNumber);
				}
				if (!HasEveryRequiredField(builder))
				{
					return Fallback(ContentLoadError::MissingField, lineNumber);
				}
				if (!stageIds.insert(builder.stage.id).second)
				{
					return Fallback(ContentLoadError::DuplicateStageId, lineNumber);
				}
				if (!ValidateStageDefinition(builder.stage).IsValid())
				{
					return Fallback(ContentLoadError::StageValidationFailed, lineNumber);
				}
				stages.push_back(std::move(builder.stage));
				insideStage = false;
				continue;
			}

			const std::size_t equals = line.find('=');
			if (equals == std::string_view::npos)
			{
				return Fallback(ContentLoadError::InvalidValue, lineNumber);
			}
			const std::string_view key = Trim(line.substr(0, equals));
			const std::string_view value = Trim(line.substr(equals + 1));
			if (!insideStage)
			{
				int version = 0;
				if (key != "version" || versionSeen)
				{
					return Fallback(key == "version" ? ContentLoadError::DuplicateKey : ContentLoadError::UnknownKey, lineNumber);
				}
				if (!ParseNumber(value, version) || version != 1)
				{
					return Fallback(ContentLoadError::UnsupportedVersion, lineNumber);
				}
				versionSeen = true;
				continue;
			}

			if (key != "peg_type" && key != "action" && key != "enemy"
				&& !builder.scalarKeys.insert(std::string(key)).second)
			{
				return Fallback(ContentLoadError::DuplicateKey, lineNumber);
			}

			if (key == "id")
			{
				if (!IsSafeStageId(value)) return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.stage.id.assign(value);
				builder.hasId = true;
			}
			else if (key == "name")
			{
				if (value.empty() || value.size() > MAX_STAGE_NAME_LENGTH) return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.stage.displayName.assign(value);
				builder.hasName = true;
			}
			else if (key == "layout")
			{
				const auto values = Split(value, ',');
				int columns = 0;
				int rows = 0;
				float startX = 0.0f;
				float startY = 0.0f;
				float spacing = 0.0f;
				float jitter = 0.0f;
				std::uint32_t seed = 0;
				if (values.size() != 7
					|| !ParseNumber(values[0], columns)
					|| !ParseNumber(values[1], rows)
					|| !ParseFiniteFloat(values[2], startX)
					|| !ParseFiniteFloat(values[3], startY)
					|| !ParseFiniteFloat(values[4], spacing)
					|| !ParseFiniteFloat(values[5], jitter)
					|| !ParseNumber(values[6], seed)
					|| columns <= 0 || rows <= 0 || columns > 256 || rows > 256
					|| static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows) > 256
					|| spacing <= 0.0f || jitter < 0.0f)
				{
					return Fallback(ContentLoadError::InvalidValue, lineNumber);
				}
				builder.stage.pegLayout = CreateSeededPegLayout(
					columns, rows, { startX, startY }, spacing, jitter, seed);
				builder.hasLayout = true;
			}
			else if (key == "peg_type")
			{
				const auto values = Split(value, ',');
				std::size_t index = 0;
				PegType type = PegType::Normal;
				if (!builder.hasLayout || values.size() != 2
					|| !ParseNumber(values[0], index)
					|| index >= builder.stage.pegLayout.pegs.size()
					|| !ParsePegType(values[1], type))
				{
					return Fallback(ContentLoadError::InvalidValue, lineNumber);
				}
				if (!builder.pegOverrideIndexes.insert(index).second)
				{
					return Fallback(ContentLoadError::DuplicateKey, lineNumber);
				}
				builder.stage.pegLayout.pegs[index].type = type;
			}
			else if (key == "player_health")
			{
				if (!ParseFiniteFloat(value, builder.stage.rules.playerHealth)) return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.hasPlayerHealth = true;
			}
			else if (key == "enemy_health")
			{
				if (!ParseFiniteFloat(value, builder.stage.rules.enemyHealth)) return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.hasEnemyHealth = true;
			}
			else if (key == "player_damage")
			{
				if (!ParseFiniteFloat(value, builder.stage.rules.playerDamage)) return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.hasPlayerDamage = true;
			}
			else if (key == "enemy_steps")
			{
				if (!ParseNumber(value, builder.stage.rules.enemyStepsBeforeAttack)) return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.hasEnemySteps = true;
			}
			else if (key == "enemy_step")
			{
				if (!ParseFiniteFloat(value, builder.stage.rules.enemyStep)) return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.hasEnemyStep = true;
			}
			else if (key == "restitution")
			{
				if (!ParseFiniteFloat(value, builder.stage.rules.pegRestitution)) return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.hasRestitution = true;
			}
			else if (key == "boss")
			{
				if (value == "true") builder.stage.isBoss = true;
				else if (value == "false") builder.stage.isBoss = false;
				else return Fallback(ContentLoadError::InvalidValue, lineNumber);
				builder.hasBoss = true;
			}
			else if (key == "action")
			{
				const auto values = Split(value, ',');
				EnemyActionDefinition action;
				if (values.size() != 2
					|| !ParseActionType(values[0], action.type)
					|| !ParseFiniteFloat(values[1], action.magnitude))
				{
					return Fallback(ContentLoadError::InvalidValue, lineNumber);
				}
				builder.stage.enemyPattern.push_back(action);
			}
			else if (key == "enemy")
			{
				const auto values = Split(value, ',');
				EnemyDefinition enemy;
				if (values.size() != 4
					|| values[0].empty()
					|| values[1].empty()
					|| !ParseEnemyVisual(values[2], enemy.visual)
					|| !ParseFiniteFloat(values[3], enemy.health))
				{
					return Fallback(ContentLoadError::InvalidValue, lineNumber);
				}
				enemy.id.assign(values[0]);
				enemy.displayName.assign(values[1]);
				builder.stage.enemies.push_back(std::move(enemy));
			}
			else
			{
				return Fallback(ContentLoadError::UnknownKey, lineNumber);
			}
		}

		if (insideStage)
		{
			return Fallback(ContentLoadError::UnexpectedSection, lineNumber);
		}
		if (!versionSeen)
		{
			return Fallback(ContentLoadError::MissingField, 0);
		}
		if (stages.empty())
		{
			return Fallback(ContentLoadError::MissingField, lineNumber);
		}

		ContentLoadResult result;
		result.state = ContentLoadState::External;
		result.stages = std::move(stages);
		return result;
	}
}

std::vector<StageDefinition> CreateBuiltInContentCatalog()
{
	return CreateBuiltInStageDefinitions();
}

ContentLoadResult LoadContentCatalog(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		std::error_code error;
		const bool exists = std::filesystem::exists(path, error);
		return Fallback(!error && !exists
			? ContentLoadError::MissingFile
			: ContentLoadError::IoFailure);
	}

	input.seekg(0, std::ios::end);
	const std::streamoff byteCount = input.tellg();
	if (byteCount < 0)
	{
		return Fallback(ContentLoadError::IoFailure);
	}
	if (static_cast<std::uintmax_t>(byteCount) > MAX_CONTENT_FILE_BYTES)
	{
		return Fallback(ContentLoadError::FileTooLarge);
	}
	input.seekg(0, std::ios::beg);
	std::string content{
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>() };
	if (!input.good() && !input.eof())
	{
		return Fallback(ContentLoadError::IoFailure);
	}
	return ParseContent(content);
}

const StageDefinition* FindContentStage(
	const std::vector<StageDefinition>& stages,
	std::string_view stageId) noexcept
{
	for (const StageDefinition& stage : stages)
	{
		if (stage.id == stageId)
		{
			return &stage;
		}
	}
	return nullptr;
}
