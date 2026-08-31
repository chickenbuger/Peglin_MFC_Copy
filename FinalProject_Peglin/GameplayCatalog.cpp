#include "pch.h"
#include "GameplayCatalog.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
	constexpr std::size_t MAX_FILE_BYTES = 128 * 1024;
	constexpr std::size_t MAX_DEFINITIONS_PER_KIND = 64;

	enum class EffectKind
	{
		PegDamageMultiplier,
		ScoreMultiplier,
		IncomingDamageMultiplier,
		EnemyDamageTakenMultiplier
	};

	struct EffectDefinition
	{
		std::string id;
		EffectKind kind = EffectKind::PegDamageMultiplier;
		float value = 1.0f;
		std::vector<std::string> includes;
		std::size_t line = 0;
	};

	struct EffectResolution
	{
		float pegDamageMultiplier = 1.0f;
		float scoreMultiplier = 1.0f;
		float incomingDamageMultiplier = 1.0f;
		float enemyDamageTakenMultiplier = 1.0f;
		bool hasPegDamage = false;
		bool hasScore = false;
		bool hasIncomingDamage = false;
		bool hasEnemyDamageTaken = false;
	};

	struct OrbEntry
	{
		OrbDefinition definition;
		std::vector<std::string> effectIds;
		std::size_t line = 0;
	};

	struct RelicEntry
	{
		RelicDefinition definition;
		std::vector<std::string> effectIds;
		std::size_t line = 0;
	};

	struct EnemyEntry
	{
		std::string id;
		std::vector<std::string> effectIds;
		std::size_t line = 0;
	};

	enum class Section
	{
		None,
		Effect,
		Orb,
		Relic,
		Enemy
	};

	struct SectionBuilder
	{
		Section section = Section::None;
		EffectDefinition effect;
		OrbEntry orb;
		RelicEntry relic;
		EnemyEntry enemy;
		std::unordered_set<std::string> scalarKeys;
	};

	GameplayCatalogLoadResult Fallback(
		GameplayCatalogLoadError error,
		std::size_t line = 0)
	{
		GameplayCatalogLoadResult result;
		result.error = error;
		result.errorLine = line;
		result.catalog.progression = CreateBuiltInProgressionCatalog();
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

	bool ParseEffectKind(std::string_view text, EffectKind& kind) noexcept
	{
		if (text == "PegDamageMultiplier") kind = EffectKind::PegDamageMultiplier;
		else if (text == "ScoreMultiplier") kind = EffectKind::ScoreMultiplier;
		else if (text == "IncomingDamageMultiplier") kind = EffectKind::IncomingDamageMultiplier;
		else if (text == "EnemyDamageTakenMultiplier") kind = EffectKind::EnemyDamageTakenMultiplier;
		else return false;
		return true;
	}

	bool ParseAttackDelivery(std::string_view text, AttackDelivery& delivery) noexcept
	{
		if (text == "Projectile") delivery = AttackDelivery::Projectile;
		else if (text == "Melee") delivery = AttackDelivery::Melee;
		else return false;
		return true;
	}

	bool ParseAttackTarget(std::string_view text, AttackTarget& target) noexcept
	{
		if (text == "Single") target = AttackTarget::Single;
		else if (text == "All") target = AttackTarget::All;
		else return false;
		return true;
	}

	bool ParseDuplicatePolicy(std::string_view text, RelicDuplicatePolicy& policy) noexcept
	{
		if (text == "Unique") policy = RelicDuplicatePolicy::Unique;
		else if (text == "Stackable") policy = RelicDuplicatePolicy::Stackable;
		else return false;
		return true;
	}

	void Combine(EffectResolution& target, const EffectResolution& source) noexcept
	{
		target.pegDamageMultiplier *= source.pegDamageMultiplier;
		target.scoreMultiplier *= source.scoreMultiplier;
		target.incomingDamageMultiplier *= source.incomingDamageMultiplier;
		target.enemyDamageTakenMultiplier *= source.enemyDamageTakenMultiplier;
		target.hasPegDamage = target.hasPegDamage || source.hasPegDamage;
		target.hasScore = target.hasScore || source.hasScore;
		target.hasIncomingDamage = target.hasIncomingDamage || source.hasIncomingDamage;
		target.hasEnemyDamageTaken = target.hasEnemyDamageTaken || source.hasEnemyDamageTaken;
	}

	bool IsResolvedValueValid(float value) noexcept
	{
		return std::isfinite(value) && value >= 0.1f && value <= 5.0f;
	}

	GameplayCatalogLoadResult ParseGameplayCatalog(
		std::string_view content,
		const std::vector<StageDefinition>& stages)
	{
		if (!IsValidUtf8(content))
		{
			return Fallback(GameplayCatalogLoadError::InvalidEncoding);
		}

		std::vector<EffectDefinition> effects;
		std::vector<OrbEntry> orbs;
		std::vector<RelicEntry> relics;
		std::vector<EnemyEntry> enemies;
		std::unordered_set<std::string> effectIds;
		std::unordered_set<std::string> orbIds;
		std::unordered_set<std::string> relicIds;
		std::unordered_set<std::string> enemyIds;
		SectionBuilder builder;
		bool versionSeen = false;
		std::size_t lineNumber = 0;

		auto CloseSection = [&](Section section) -> GameplayCatalogLoadResult
		{
			if (builder.section != section)
			{
				return Fallback(GameplayCatalogLoadError::UnexpectedSection, lineNumber);
			}
			switch (section)
			{
			case Section::Effect:
				if (builder.effect.id.empty()
					|| !builder.scalarKeys.contains("kind")
					|| !builder.scalarKeys.contains("value"))
				{
					return Fallback(GameplayCatalogLoadError::MissingField, lineNumber);
				}
				if (!effectIds.insert(builder.effect.id).second)
				{
					return Fallback(GameplayCatalogLoadError::DuplicateId, lineNumber);
				}
				effects.push_back(std::move(builder.effect));
				break;
			case Section::Orb:
				if (builder.orb.definition.id.empty()
					|| builder.orb.definition.displayName.empty()
					|| !builder.scalarKeys.contains("delivery")
					|| !builder.scalarKeys.contains("target")
					|| builder.orb.effectIds.empty())
				{
					return Fallback(GameplayCatalogLoadError::MissingField, lineNumber);
				}
				if (!orbIds.insert(builder.orb.definition.id).second)
				{
					return Fallback(GameplayCatalogLoadError::DuplicateId, lineNumber);
				}
				orbs.push_back(std::move(builder.orb));
				break;
			case Section::Relic:
				if (builder.relic.definition.id.empty()
					|| builder.relic.definition.displayName.empty()
					|| !builder.scalarKeys.contains("duplicate")
					|| !builder.scalarKeys.contains("max_stacks")
					|| builder.relic.effectIds.empty())
				{
					return Fallback(GameplayCatalogLoadError::MissingField, lineNumber);
				}
				if (!relicIds.insert(builder.relic.definition.id).second)
				{
					return Fallback(GameplayCatalogLoadError::DuplicateId, lineNumber);
				}
				relics.push_back(std::move(builder.relic));
				break;
			case Section::Enemy:
				if (builder.enemy.id.empty() || builder.enemy.effectIds.empty())
				{
					return Fallback(GameplayCatalogLoadError::MissingField, lineNumber);
				}
				if (!enemyIds.insert(builder.enemy.id).second)
				{
					return Fallback(GameplayCatalogLoadError::DuplicateId, lineNumber);
				}
				enemies.push_back(std::move(builder.enemy));
				break;
			case Section::None:
				return Fallback(GameplayCatalogLoadError::UnexpectedSection, lineNumber);
			}
			builder = {};
			return {};
		};

		while (!content.empty())
		{
			++lineNumber;
			const std::size_t newline = content.find('\n');
			const std::string_view line = Trim(content.substr(0, newline));
			if (newline == std::string_view::npos) content = {};
			else content.remove_prefix(newline + 1);
			if (line.empty() || line.front() == '#')
			{
				continue;
			}

			Section opening = Section::None;
			if (line == "[effect]") opening = Section::Effect;
			else if (line == "[orb]") opening = Section::Orb;
			else if (line == "[relic]") opening = Section::Relic;
			else if (line == "[enemy]") opening = Section::Enemy;
			if (opening != Section::None)
			{
				if (!versionSeen || builder.section != Section::None)
				{
					return Fallback(GameplayCatalogLoadError::UnexpectedSection, lineNumber);
				}
				builder.section = opening;
				builder.effect.line = lineNumber;
				builder.orb.line = lineNumber;
				builder.relic.line = lineNumber;
				builder.enemy.line = lineNumber;
				continue;
			}

			Section closing = Section::None;
			if (line == "[/effect]") closing = Section::Effect;
			else if (line == "[/orb]") closing = Section::Orb;
			else if (line == "[/relic]") closing = Section::Relic;
			else if (line == "[/enemy]") closing = Section::Enemy;
			if (closing != Section::None)
			{
				const GameplayCatalogLoadResult closed = CloseSection(closing);
				if (closed.error != GameplayCatalogLoadError::None)
				{
					return closed;
				}
				continue;
			}

			const std::size_t equals = line.find('=');
			if (equals == std::string_view::npos)
			{
				return Fallback(GameplayCatalogLoadError::InvalidValue, lineNumber);
			}
			const std::string_view key = Trim(line.substr(0, equals));
			const std::string_view value = Trim(line.substr(equals + 1));
			if (builder.section == Section::None)
			{
				int version = 0;
				if (key != "version" || versionSeen)
				{
					return Fallback(key == "version"
						? GameplayCatalogLoadError::DuplicateKey
						: GameplayCatalogLoadError::UnknownKey, lineNumber);
				}
				if (!ParseNumber(value, version) || version != 1)
				{
					return Fallback(GameplayCatalogLoadError::UnsupportedVersion, lineNumber);
				}
				versionSeen = true;
				continue;
			}

			const bool repeatedReference = (builder.section == Section::Effect && key == "include")
				|| ((builder.section == Section::Orb
					|| builder.section == Section::Relic
					|| builder.section == Section::Enemy) && key == "effect");
			if (!repeatedReference && !builder.scalarKeys.insert(std::string(key)).second)
			{
				return Fallback(GameplayCatalogLoadError::DuplicateKey, lineNumber);
			}

			if (builder.section == Section::Effect)
			{
				if (key == "id" && IsSafeId(value)) builder.effect.id.assign(value);
				else if (key == "kind" && ParseEffectKind(value, builder.effect.kind)) {}
				else if (key == "value"
					&& ParseNumber(value, builder.effect.value)
					&& IsResolvedValueValid(builder.effect.value)) {}
				else if (key == "include" && IsSafeId(value)) builder.effect.includes.emplace_back(value);
				else return Fallback(key == "id" || key == "kind" || key == "value" || key == "include"
					? GameplayCatalogLoadError::InvalidValue
					: GameplayCatalogLoadError::UnknownKey, lineNumber);
			}
			else if (builder.section == Section::Orb)
			{
				if (key == "id" && IsSafeId(value)) builder.orb.definition.id.assign(value);
				else if (key == "name" && !value.empty() && value.size() <= 80) builder.orb.definition.displayName.assign(value);
				else if (key == "delivery" && ParseAttackDelivery(value, builder.orb.definition.attackDelivery)) {}
				else if (key == "target" && ParseAttackTarget(value, builder.orb.definition.attackTarget)) {}
				else if (key == "effect" && IsSafeId(value)) builder.orb.effectIds.emplace_back(value);
				else return Fallback(key == "id" || key == "name" || key == "delivery" || key == "target" || key == "effect"
					? GameplayCatalogLoadError::InvalidValue
					: GameplayCatalogLoadError::UnknownKey, lineNumber);
			}
			else if (builder.section == Section::Relic)
			{
				if (key == "id" && IsSafeId(value)) builder.relic.definition.id.assign(value);
				else if (key == "name" && !value.empty() && value.size() <= 80) builder.relic.definition.displayName.assign(value);
				else if (key == "duplicate" && ParseDuplicatePolicy(value, builder.relic.definition.duplicatePolicy)) {}
				else if (key == "max_stacks"
					&& ParseNumber(value, builder.relic.definition.maxStacks)
					&& builder.relic.definition.maxStacks > 0
					&& builder.relic.definition.maxStacks <= 8) {}
				else if (key == "effect" && IsSafeId(value)) builder.relic.effectIds.emplace_back(value);
				else return Fallback(key == "id" || key == "name" || key == "duplicate" || key == "max_stacks" || key == "effect"
					? GameplayCatalogLoadError::InvalidValue
					: GameplayCatalogLoadError::UnknownKey, lineNumber);
			}
			else if (builder.section == Section::Enemy)
			{
				if (key == "id" && IsSafeId(value)) builder.enemy.id.assign(value);
				else if (key == "effect" && IsSafeId(value)) builder.enemy.effectIds.emplace_back(value);
				else return Fallback(key == "id" || key == "effect"
					? GameplayCatalogLoadError::InvalidValue
					: GameplayCatalogLoadError::UnknownKey, lineNumber);
			}
		}

		if (builder.section != Section::None)
		{
			return Fallback(GameplayCatalogLoadError::UnexpectedSection, lineNumber);
		}
		if (!versionSeen || effects.empty() || orbs.empty() || relics.empty() || enemies.empty()
			|| effects.size() > MAX_DEFINITIONS_PER_KIND
			|| orbs.size() > MAX_DEFINITIONS_PER_KIND
			|| relics.size() > MAX_DEFINITIONS_PER_KIND
			|| enemies.size() > MAX_DEFINITIONS_PER_KIND)
		{
			return Fallback(GameplayCatalogLoadError::MissingField, lineNumber);
		}

		std::unordered_map<std::string, std::size_t> effectIndexes;
		for (std::size_t index = 0; index < effects.size(); ++index)
		{
			effectIndexes.emplace(effects[index].id, index);
		}
		std::vector<unsigned char> visitState(effects.size(), 0);
		std::vector<EffectResolution> resolutions(effects.size());
		GameplayCatalogLoadError resolutionError = GameplayCatalogLoadError::None;
		std::size_t resolutionErrorLine = 0;
		auto Resolve = [&](auto&& self, std::size_t index) -> bool
		{
			if (visitState[index] == 2) return true;
			if (visitState[index] == 1)
			{
				resolutionError = GameplayCatalogLoadError::CircularEffectReference;
				resolutionErrorLine = effects[index].line;
				return false;
			}
			visitState[index] = 1;
			EffectResolution resolved;
			for (const std::string& includedId : effects[index].includes)
			{
				const auto included = effectIndexes.find(includedId);
				if (included == effectIndexes.end())
				{
					resolutionError = GameplayCatalogLoadError::UnknownEffectReference;
					resolutionErrorLine = effects[index].line;
					return false;
				}
				if (!self(self, included->second)) return false;
				Combine(resolved, resolutions[included->second]);
			}
			switch (effects[index].kind)
			{
			case EffectKind::PegDamageMultiplier:
				resolved.pegDamageMultiplier *= effects[index].value;
				resolved.hasPegDamage = true;
				break;
			case EffectKind::ScoreMultiplier:
				resolved.scoreMultiplier *= effects[index].value;
				resolved.hasScore = true;
				break;
			case EffectKind::IncomingDamageMultiplier:
				resolved.incomingDamageMultiplier *= effects[index].value;
				resolved.hasIncomingDamage = true;
				break;
			case EffectKind::EnemyDamageTakenMultiplier:
				resolved.enemyDamageTakenMultiplier *= effects[index].value;
				resolved.hasEnemyDamageTaken = true;
				break;
			}
			if (!IsResolvedValueValid(resolved.pegDamageMultiplier)
				|| !IsResolvedValueValid(resolved.scoreMultiplier)
				|| !IsResolvedValueValid(resolved.incomingDamageMultiplier)
				|| !IsResolvedValueValid(resolved.enemyDamageTakenMultiplier))
			{
				resolutionError = GameplayCatalogLoadError::InvalidResolvedEffect;
				resolutionErrorLine = effects[index].line;
				return false;
			}
			resolutions[index] = resolved;
			visitState[index] = 2;
			return true;
		};
		for (std::size_t index = 0; index < effects.size(); ++index)
		{
			if (!Resolve(Resolve, index))
			{
				return Fallback(resolutionError, resolutionErrorLine);
			}
		}

		auto ResolveReferences = [&](const std::vector<std::string>& ids, EffectResolution& result) -> bool
		{
			std::unordered_set<std::string> uniqueIds;
			for (const std::string& id : ids)
			{
				const auto found = effectIndexes.find(id);
				if (found == effectIndexes.end() || !uniqueIds.insert(id).second)
				{
					return false;
				}
				Combine(result, resolutions[found->second]);
			}
			return true;
		};

		GameplayCatalog catalog;
		for (OrbEntry& entry : orbs)
		{
			EffectResolution resolved;
			if (!ResolveReferences(entry.effectIds, resolved)
				|| resolved.hasIncomingDamage
				|| resolved.hasEnemyDamageTaken)
			{
				return Fallback(GameplayCatalogLoadError::UnknownEffectReference, entry.line);
			}
			entry.definition.pegDamageMultiplier = resolved.pegDamageMultiplier;
			entry.definition.scoreMultiplier = resolved.scoreMultiplier;
			catalog.progression.orbs.push_back(std::move(entry.definition));
		}
		for (RelicEntry& entry : relics)
		{
			EffectResolution resolved;
			if (!ResolveReferences(entry.effectIds, resolved) || resolved.hasEnemyDamageTaken)
			{
				return Fallback(GameplayCatalogLoadError::UnknownEffectReference, entry.line);
			}
			entry.definition.pegDamageMultiplier = resolved.pegDamageMultiplier;
			entry.definition.scoreMultiplier = resolved.scoreMultiplier;
			entry.definition.incomingDamageMultiplier = resolved.incomingDamageMultiplier;
			catalog.progression.relics.push_back(std::move(entry.definition));
		}
		for (const EnemyEntry& entry : enemies)
		{
			EffectResolution resolved;
			if (!ResolveReferences(entry.effectIds, resolved)
				|| resolved.hasPegDamage
				|| resolved.hasScore
				|| resolved.hasIncomingDamage
				|| !resolved.hasEnemyDamageTaken)
			{
				return Fallback(GameplayCatalogLoadError::UnknownEffectReference, entry.line);
			}
			catalog.enemies.push_back({ entry.id, resolved.enemyDamageTakenMultiplier });
		}

		if (!ValidateProgressionCatalog(catalog.progression))
		{
			return Fallback(GameplayCatalogLoadError::InvalidValue, lineNumber);
		}

		for (const StageDefinition& stage : stages)
		{
			for (const EnemyDefinition& enemy : stage.enemies)
			{
				const auto found = std::find_if(
					catalog.enemies.begin(),
					catalog.enemies.end(),
					[&enemy](const EnemyEffectBinding& binding)
					{
						return binding.enemyId == enemy.id;
					});
				if (found == catalog.enemies.end())
				{
					return Fallback(GameplayCatalogLoadError::UnknownEnemyReference, lineNumber);
				}
			}
		}

		GameplayCatalogLoadResult result;
		result.state = GameplayCatalogLoadState::External;
		result.catalog = std::move(catalog);
		return result;
	}
}

GameplayCatalogLoadResult LoadGameplayCatalog(
	const std::filesystem::path& path,
	const std::vector<StageDefinition>& stages)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
	{
		std::error_code error;
		const bool exists = std::filesystem::exists(path, error);
		return Fallback(!error && !exists
			? GameplayCatalogLoadError::MissingFile
			: GameplayCatalogLoadError::IoFailure);
	}
	input.seekg(0, std::ios::end);
	const std::streamoff byteCount = input.tellg();
	if (byteCount < 0) return Fallback(GameplayCatalogLoadError::IoFailure);
	if (static_cast<std::uintmax_t>(byteCount) > MAX_FILE_BYTES)
	{
		return Fallback(GameplayCatalogLoadError::FileTooLarge);
	}
	input.seekg(0, std::ios::beg);
	std::string content{
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>() };
	if (!input.good() && !input.eof())
	{
		return Fallback(GameplayCatalogLoadError::IoFailure);
	}
	return ParseGameplayCatalog(content, stages);
}

bool ActivateGameplayCatalog(
	const GameplayCatalogLoadResult& result,
	std::vector<StageDefinition>& stages)
{
	if (!InstallProgressionCatalog(result.catalog.progression))
	{
		ResetProgressionCatalog();
		return false;
	}
	if (!result.UsedExternalContent())
	{
		return true;
	}

	for (StageDefinition& stage : stages)
	{
		for (EnemyDefinition& enemy : stage.enemies)
		{
			const auto found = std::find_if(
				result.catalog.enemies.begin(),
				result.catalog.enemies.end(),
				[&enemy](const EnemyEffectBinding& binding)
				{
					return binding.enemyId == enemy.id;
				});
			if (found == result.catalog.enemies.end())
			{
				ResetProgressionCatalog();
				return false;
			}
			enemy.damageTakenMultiplier = found->damageTakenMultiplier;
		}
		if (!ValidateStageDefinition(stage).IsValid())
		{
			ResetProgressionCatalog();
			return false;
		}
	}
	return true;
}
