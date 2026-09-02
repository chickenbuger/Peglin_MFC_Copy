#include "pch.h"
#include "Progression.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <utility>

namespace
{
	const ProgressionCatalog BUILT_IN_CATALOG = {
		{
			{ "basic-orb", "Traveler Orb", "orb-traveler-v1", 1.0f, 1.0f, AttackDelivery::Projectile, AttackTarget::Single },
			{ "iron-orb", "Iron Orb", "orb-iron-v1", 1.5f, 0.75f, AttackDelivery::Melee, AttackTarget::Single },
			{ "echo-orb", "Echo Orb", "orb-echo-v1", 0.8f, 1.5f, AttackDelivery::Projectile, AttackTarget::All },
			{ "cinder-orb", "Cinder Orb", "orb-cinder-v1", 1.3f, 1.0f, AttackDelivery::Projectile, AttackTarget::Single },
			{ "verdant-orb", "Verdant Orb", "orb-verdant-v1", 0.9f, 1.25f, AttackDelivery::Projectile, AttackTarget::All }
		},
		{
			{ "combo-lantern", "Combo Lantern", "relic-combo-lantern-v1", RelicDuplicatePolicy::Unique, 1, 1.0f, 1.25f, 1.0f },
			{ "thorn-charm", "Thorn Charm", "relic-thorn-charm-v1", RelicDuplicatePolicy::Unique, 1, 1.2f, 1.0f, 1.0f },
			{ "bark-guard", "Bark Guard", "relic-bark-guard-v1", RelicDuplicatePolicy::Stackable, 2, 1.0f, 1.0f, 0.85f },
			{ "ember-heart", "Ember Heart", "relic-ember-heart-v1", RelicDuplicatePolicy::Unique, 1, 1.15f, 1.0f, 1.10f },
			{ "wayfinder-compass", "Wayfinder Compass", "relic-wayfinder-v1", RelicDuplicatePolicy::Unique, 1, 1.0f, 1.10f, 0.95f }
		}
	};

	ProgressionCatalog ACTIVE_CATALOG = BUILT_IN_CATALOG;

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

	bool IsValidMultiplier(float value) noexcept
	{
		return std::isfinite(value) && value >= 0.1f && value <= 5.0f;
	}

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
	return ACTIVE_CATALOG.orbs;
}

const std::vector<RelicDefinition>& GetRelicDefinitions() noexcept
{
	return ACTIVE_CATALOG.relics;
}

const OrbDefinition* FindOrbDefinition(std::string_view id) noexcept
{
	const auto found = std::find_if(
		ACTIVE_CATALOG.orbs.begin(),
		ACTIVE_CATALOG.orbs.end(),
		[id](const OrbDefinition& definition)
		{
			return definition.id == id;
		});
	return found == ACTIVE_CATALOG.orbs.end() ? nullptr : &*found;
}

const RelicDefinition* FindRelicDefinition(std::string_view id) noexcept
{
	const auto found = std::find_if(
		ACTIVE_CATALOG.relics.begin(),
		ACTIVE_CATALOG.relics.end(),
		[id](const RelicDefinition& definition)
		{
			return definition.id == id;
		});
	return found == ACTIVE_CATALOG.relics.end() ? nullptr : &*found;
}

ProgressionCatalog CreateBuiltInProgressionCatalog()
{
	return BUILT_IN_CATALOG;
}

bool ValidateProgressionCatalog(const ProgressionCatalog& catalog) noexcept
{
	if (catalog.orbs.empty() || catalog.relics.empty())
	{
		return false;
	}

	std::unordered_set<std::string> orbIds;
	for (const OrbDefinition& orb : catalog.orbs)
	{
		if (!IsSafeId(orb.id)
			|| orb.displayName.empty()
			|| !IsSafeId(orb.imageKey)
			|| !IsValidMultiplier(orb.pegDamageMultiplier)
			|| !IsValidMultiplier(orb.scoreMultiplier)
			|| !orbIds.insert(orb.id).second)
		{
			return false;
		}
	}
	for (const std::string_view starterId : { "basic-orb", "iron-orb", "echo-orb" })
	{
		if (!orbIds.contains(std::string(starterId)))
		{
			return false;
		}
	}

	std::unordered_set<std::string> relicIds;
	for (const RelicDefinition& relic : catalog.relics)
	{
		if (!IsSafeId(relic.id)
			|| relic.displayName.empty()
			|| !IsSafeId(relic.imageKey)
			|| relic.maxStacks == 0
			|| !IsValidMultiplier(relic.pegDamageMultiplier)
			|| !IsValidMultiplier(relic.scoreMultiplier)
			|| !IsValidMultiplier(relic.incomingDamageMultiplier)
			|| !relicIds.insert(relic.id).second)
		{
			return false;
		}
	}

	return true;
}

bool InstallProgressionCatalog(const ProgressionCatalog& catalog)
{
	if (!ValidateProgressionCatalog(catalog))
	{
		return false;
	}
	ACTIVE_CATALOG = catalog;
	return true;
}

void ResetProgressionCatalog()
{
	ACTIVE_CATALOG = BUILT_IN_CATALOG;
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
	return definition == nullptr ? ACTIVE_CATALOG.orbs.front() : *definition;
}

const OrbDefinition& PlayerLoadout::GetNextOrb() const noexcept
{
	const OrbDefinition* definition = FindOrbDefinition(GetNextOrbId());
	return definition == nullptr ? ACTIVE_CATALOG.orbs.front() : *definition;
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

PlayerLoadoutSnapshot PlayerLoadout::CreatePersistentSnapshot() const
{
	return { _ownedOrbIds, _preferredOrbId, _acquiredRelics };
}

bool PlayerLoadout::RestorePersistentSnapshot(const PlayerLoadoutSnapshot& snapshot)
{
	if (snapshot.ownedOrbIds.empty()
		|| snapshot.ownedOrbIds.size() > MaxOwnedOrbs
		|| FindOrbDefinition(snapshot.preferredOrbId) == nullptr
		|| std::find(
			snapshot.ownedOrbIds.begin(),
			snapshot.ownedOrbIds.end(),
			snapshot.preferredOrbId) == snapshot.ownedOrbIds.end())
	{
		return false;
	}
	for (const std::string& orbId : snapshot.ownedOrbIds)
	{
		if (FindOrbDefinition(orbId) == nullptr)
		{
			return false;
		}
	}

	for (const std::string& relicId : snapshot.acquiredRelics)
	{
		const RelicDefinition* definition = FindRelicDefinition(relicId);
		if (definition == nullptr)
		{
			return false;
		}
		const std::size_t stackCount = static_cast<std::size_t>(std::count(
			snapshot.acquiredRelics.begin(),
			snapshot.acquiredRelics.end(),
			relicId));
		if (stackCount > definition->maxStacks
			|| (definition->duplicatePolicy == RelicDuplicatePolicy::Unique
				&& stackCount > 1))
		{
			return false;
		}
	}

	_ownedOrbIds = snapshot.ownedOrbIds;
	_preferredOrbId = snapshot.preferredOrbId;
	_acquiredRelics = snapshot.acquiredRelics;
	BeginBattle(0u);
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
