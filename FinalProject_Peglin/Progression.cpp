#include "pch.h"
#include "Progression.h"

#include <algorithm>
#include <utility>

namespace
{
	const std::vector<OrbDefinition> ORB_DEFINITIONS = {
		{ "basic-orb", "Traveler Orb", 1.0f, 1.0f, AttackDelivery::Projectile, AttackTarget::Single },
		{ "iron-orb", "Iron Orb", 1.5f, 0.75f, AttackDelivery::Melee, AttackTarget::Single },
		{ "echo-orb", "Echo Orb", 0.8f, 1.5f, AttackDelivery::Projectile, AttackTarget::All }
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

PlayerLoadout::PlayerLoadout()
{
	Reset();
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
	if (FindOrbDefinition(id) == nullptr
		|| std::find(_ownedOrbIds.begin(), _ownedOrbIds.end(), id) == _ownedOrbIds.end())
	{
		return false;
	}

	_preferredOrbId.assign(id);
	const auto found = std::find(
		_currentCycle.begin() + static_cast<std::ptrdiff_t>(_cycleIndex),
		_currentCycle.end(),
		id);
	if (found != _currentCycle.end())
	{
		std::iter_swap(
			_currentCycle.begin() + static_cast<std::ptrdiff_t>(_cycleIndex),
			found);
	}
	return true;
}

bool PlayerLoadout::AddOrb(std::string_view id)
{
	if (FindOrbDefinition(id) == nullptr || _ownedOrbIds.size() >= MaxOwnedOrbs)
	{
		return false;
	}

	_ownedOrbIds.emplace_back(id);
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
	_ownedOrbIds = { "basic-orb", "iron-orb", "echo-orb" };
	_preferredOrbId.assign(DefaultOrbId);
	_acquiredRelics.clear();
	BeginBattle(0u);
}

const OrbDefinition& PlayerLoadout::GetSelectedOrb() const noexcept
{
	const OrbDefinition* definition = FindOrbDefinition(GetSelectedOrbId());
	return definition == nullptr ? ORB_DEFINITIONS.front() : *definition;
}

const OrbDefinition& PlayerLoadout::GetNextOrb() const noexcept
{
	const OrbDefinition* definition = FindOrbDefinition(GetNextOrbId());
	return definition == nullptr ? ORB_DEFINITIONS.front() : *definition;
}

std::string_view PlayerLoadout::GetSelectedOrbId() const noexcept
{
	return _currentCycle.empty() || _cycleIndex >= _currentCycle.size()
		? DefaultOrbId
		: std::string_view(_currentCycle[_cycleIndex]);
}

std::string_view PlayerLoadout::GetNextOrbId() const noexcept
{
	if (_cycleIndex + 1 < _currentCycle.size())
	{
		return _currentCycle[_cycleIndex + 1];
	}
	return _nextCycle.empty()
		? GetSelectedOrbId()
		: std::string_view(_nextCycle.front());
}

std::size_t PlayerLoadout::GetDrawPileCount() const noexcept
{
	return _currentCycle.empty() || _cycleIndex >= _currentCycle.size()
		? 0
		: _currentCycle.size() - _cycleIndex - 1;
}

void PlayerLoadout::BeginBattle(std::uint32_t shuffleSeed)
{
	_shuffleGenerator.seed(shuffleSeed);
	_currentCycle = BuildShuffledCycle();
	MovePreferredOrbToFront();
	_nextCycle = BuildShuffledCycle();
	_cycleIndex = 0;
	_reloadCount = 0;
}

bool PlayerLoadout::AdvanceOrb()
{
	if (_currentCycle.empty())
	{
		return false;
	}

	++_cycleIndex;
	if (_cycleIndex >= _currentCycle.size())
	{
		_currentCycle = std::move(_nextCycle);
		_nextCycle = BuildShuffledCycle();
		_cycleIndex = 0;
		++_reloadCount;
	}
	return true;
}

std::vector<std::string> PlayerLoadout::BuildShuffledCycle()
{
	std::vector<std::string> cycle = _ownedOrbIds;
	std::shuffle(cycle.begin(), cycle.end(), _shuffleGenerator);
	return cycle;
}

void PlayerLoadout::MovePreferredOrbToFront()
{
	const auto preferred = std::find(
		_currentCycle.begin(),
		_currentCycle.end(),
		_preferredOrbId);
	if (preferred != _currentCycle.end())
	{
		std::iter_swap(_currentCycle.begin(), preferred);
	}
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
