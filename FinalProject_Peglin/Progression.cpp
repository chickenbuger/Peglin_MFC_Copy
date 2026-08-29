#include "pch.h"
#include "Progression.h"

#include <algorithm>

namespace
{
	const std::vector<OrbDefinition> ORB_DEFINITIONS = {
		{ "basic-orb", "Traveler Orb", 1.0f, 1.0f },
		{ "iron-orb", "Iron Orb", 1.5f, 0.75f },
		{ "echo-orb", "Echo Orb", 0.8f, 1.5f }
	};

	const std::vector<RelicDefinition> RELIC_DEFINITIONS = {
		{
			"combo-lantern",
			"Combo Lantern",
			RelicDuplicatePolicy::Unique,
			1,
			1.0f,
			1.25f,
			1.0f
		},
		{
			"thorn-charm",
			"Thorn Charm",
			RelicDuplicatePolicy::Unique,
			1,
			1.2f,
			1.0f,
			1.0f
		},
		{
			"bark-guard",
			"Bark Guard",
			RelicDuplicatePolicy::Stackable,
			2,
			1.0f,
			1.0f,
			0.85f
		}
	};

	float ClampModifier(float value) noexcept
	{
		return std::clamp(value, 0.1f, 5.0f);
	}
}

const std::vector<OrbDefinition>& GetOrbDefinitions() noexcept
{
	return ORB_DEFINITIONS;
}

const std::vector<RelicDefinition>& GetRelicDefinitions() noexcept
{
	return RELIC_DEFINITIONS;
}

const OrbDefinition* FindOrbDefinition(std::string_view id) noexcept
{
	const auto found = std::find_if(
		ORB_DEFINITIONS.begin(),
		ORB_DEFINITIONS.end(),
		[id](const OrbDefinition& definition)
		{
			return definition.id == id;
		});
	return found == ORB_DEFINITIONS.end() ? nullptr : &*found;
}

const RelicDefinition* FindRelicDefinition(std::string_view id) noexcept
{
	const auto found = std::find_if(
		RELIC_DEFINITIONS.begin(),
		RELIC_DEFINITIONS.end(),
		[id](const RelicDefinition& definition)
		{
			return definition.id == id;
		});
	return found == RELIC_DEFINITIONS.end() ? nullptr : &*found;
}

bool PlayerLoadout::SelectOrb(std::string_view id)
{
	if (FindOrbDefinition(id) == nullptr)
	{
		return false;
	}

	_selectedOrbId.assign(id);
	return true;
}

bool PlayerLoadout::AcquireRelic(std::string_view id)
{
	const RelicDefinition* definition = FindRelicDefinition(id);
	if (definition == nullptr)
	{
		return false;
	}

	const std::size_t currentStacks = GetRelicStackCount(id);
	if (definition->duplicatePolicy == RelicDuplicatePolicy::Unique && currentStacks > 0)
	{
		return false;
	}
	if (currentStacks >= definition->maxStacks)
	{
		return false;
	}

	_acquiredRelics.emplace_back(id);
	return true;
}

void PlayerLoadout::Reset()
{
	_selectedOrbId.assign(DefaultOrbId);
	_acquiredRelics.clear();
}

const OrbDefinition& PlayerLoadout::GetSelectedOrb() const noexcept
{
	const OrbDefinition* definition = FindOrbDefinition(_selectedOrbId);
	return definition == nullptr ? ORB_DEFINITIONS.front() : *definition;
}

std::size_t PlayerLoadout::GetRelicStackCount(std::string_view id) const noexcept
{
	return static_cast<std::size_t>(std::count(
		_acquiredRelics.begin(),
		_acquiredRelics.end(),
		id));
}

ProgressionModifiers PlayerLoadout::CalculateModifiers() const noexcept
{
	const OrbDefinition& orb = GetSelectedOrb();
	ProgressionModifiers modifiers;
	modifiers.pegDamageMultiplier = orb.pegDamageMultiplier;
	modifiers.scoreMultiplier = orb.scoreMultiplier;

	for (const std::string& relicId : _acquiredRelics)
	{
		const RelicDefinition* relic = FindRelicDefinition(relicId);
		if (relic == nullptr)
		{
			continue;
		}
		modifiers.pegDamageMultiplier *= relic->pegDamageMultiplier;
		modifiers.scoreMultiplier *= relic->scoreMultiplier;
		modifiers.incomingDamageMultiplier *= relic->incomingDamageMultiplier;
	}

	modifiers.pegDamageMultiplier = ClampModifier(modifiers.pegDamageMultiplier);
	modifiers.scoreMultiplier = ClampModifier(modifiers.scoreMultiplier);
	modifiers.incomingDamageMultiplier = ClampModifier(modifiers.incomingDamageMultiplier);
	return modifiers;
}
