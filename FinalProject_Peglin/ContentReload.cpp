#include "pch.h"
#include "ContentReload.h"

#include <algorithm>

namespace
{
	template <typename Definition>
	const Definition* FindDefinition(
		const std::vector<Definition>& definitions,
		std::string_view id) noexcept
	{
		const auto found = std::find_if(
			definitions.begin(),
			definitions.end(),
			[id](const Definition& definition) { return definition.id == id; });
		return found == definitions.end() ? nullptr : &*found;
	}
}

ContentReloadResult PrepareContentReload(
	const std::filesystem::path& stageCatalogPath,
	const std::filesystem::path& gameplayCatalogPath,
	const std::vector<std::string>& requiredStageIds,
	const PlayerLoadoutSnapshot& activeLoadout)
{
	ContentReloadResult result;
	result.content = LoadContentCatalog(stageCatalogPath);
	if (!result.content.UsedExternalContent())
	{
		result.error = ContentReloadError::StageCatalog;
		result.errorLine = result.content.errorLine;
		return result;
	}

	result.gameplay = LoadGameplayCatalog(gameplayCatalogPath, result.content.stages);
	if (!result.gameplay.UsedExternalContent())
	{
		result.error = ContentReloadError::GameplayCatalog;
		result.errorLine = result.gameplay.errorLine;
		return result;
	}
	if (!ResolveGameplayCatalogStages(result.gameplay, result.content.stages))
	{
		result.error = ContentReloadError::GameplayResolution;
		return result;
	}

	result.difficulty = AnalyzeDifficultyCurve(result.content.stages);
	if (!result.difficulty.passed)
	{
		result.error = ContentReloadError::DifficultyCurve;
		return result;
	}

	for (const std::string& stageId : requiredStageIds)
	{
		if (FindContentStage(result.content.stages, stageId) == nullptr)
		{
			result.error = ContentReloadError::MissingRunStage;
			result.incompatibleId = stageId;
			return result;
		}
	}

	const auto& orbs = result.gameplay.catalog.progression.orbs;
	if (activeLoadout.ownedOrbIds.empty()
		|| std::find(
			activeLoadout.ownedOrbIds.begin(),
			activeLoadout.ownedOrbIds.end(),
			activeLoadout.preferredOrbId) == activeLoadout.ownedOrbIds.end())
	{
		result.error = ContentReloadError::MissingOrb;
		result.incompatibleId = activeLoadout.preferredOrbId;
		return result;
	}
	for (const std::string& orbId : activeLoadout.ownedOrbIds)
	{
		if (FindDefinition(orbs, orbId) == nullptr)
		{
			result.error = ContentReloadError::MissingOrb;
			result.incompatibleId = orbId;
			return result;
		}
	}

	const auto& relics = result.gameplay.catalog.progression.relics;
	for (const std::string& relicId : activeLoadout.acquiredRelics)
	{
		const RelicDefinition* definition = FindDefinition(relics, relicId);
		if (definition == nullptr)
		{
			result.error = ContentReloadError::MissingRelic;
			result.incompatibleId = relicId;
			return result;
		}
		const std::size_t count = static_cast<std::size_t>(std::count(
			activeLoadout.acquiredRelics.begin(),
			activeLoadout.acquiredRelics.end(),
			relicId));
		if (count > definition->maxStacks
			|| (definition->duplicatePolicy == RelicDuplicatePolicy::Unique && count > 1U))
		{
			result.error = ContentReloadError::MissingRelic;
			result.incompatibleId = relicId;
			return result;
		}
	}

	return result;
}
