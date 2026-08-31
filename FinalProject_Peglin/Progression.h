#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

enum class RelicDuplicatePolicy
{
	Unique,
	Stackable
};

enum class AttackDelivery
{
	Projectile,
	Melee
};

enum class AttackTarget
{
	Single,
	All
};

struct OrbDefinition
{
	std::string id;
	std::string displayName;
	std::string imageKey;
	float pegDamageMultiplier = 1.0f;
	float scoreMultiplier = 1.0f;
	AttackDelivery attackDelivery = AttackDelivery::Projectile;
	AttackTarget attackTarget = AttackTarget::Single;
};

struct RelicDefinition
{
	std::string id;
	std::string displayName;
	std::string imageKey;
	RelicDuplicatePolicy duplicatePolicy = RelicDuplicatePolicy::Unique;
	std::size_t maxStacks = 1;
	float pegDamageMultiplier = 1.0f;
	float scoreMultiplier = 1.0f;
	float incomingDamageMultiplier = 1.0f;
};

struct ProgressionModifiers
{
	float pegDamageMultiplier = 1.0f;
	float scoreMultiplier = 1.0f;
	float incomingDamageMultiplier = 1.0f;
};

struct ProgressionCatalog
{
	std::vector<OrbDefinition> orbs;
	std::vector<RelicDefinition> relics;
};

const std::vector<OrbDefinition>& GetOrbDefinitions() noexcept;
const std::vector<RelicDefinition>& GetRelicDefinitions() noexcept;
const OrbDefinition* FindOrbDefinition(std::string_view id) noexcept;
const RelicDefinition* FindRelicDefinition(std::string_view id) noexcept;
ProgressionCatalog CreateBuiltInProgressionCatalog();
bool ValidateProgressionCatalog(const ProgressionCatalog& catalog) noexcept;
bool InstallProgressionCatalog(const ProgressionCatalog& catalog);
void ResetProgressionCatalog();

class PlayerLoadout
{
public:
	static constexpr std::string_view DefaultOrbId = "basic-orb";
	static constexpr std::size_t MaxOwnedOrbs = 24;

	PlayerLoadout();

	bool SelectOrb(std::string_view id);
	bool AddOrb(std::string_view id);
	bool AcquireRelic(std::string_view id);
	void Reset();
	void BeginBattle(std::uint32_t shuffleSeed);
	bool AdvanceOrb();

	const OrbDefinition& GetSelectedOrb() const noexcept;
	const OrbDefinition& GetNextOrb() const noexcept;
	std::string_view GetSelectedOrbId() const noexcept;
	std::string_view GetNextOrbId() const noexcept;
	const std::vector<std::string>& GetOwnedOrbs() const noexcept { return _ownedOrbIds; }
	std::size_t GetDrawPileCount() const noexcept;
	std::size_t GetDiscardPileCount() const noexcept { return _cycleIndex; }
	std::size_t GetReloadCount() const noexcept { return _reloadCount; }
	std::size_t GetRelicStackCount(std::string_view id) const noexcept;
	const std::vector<std::string>& GetAcquiredRelics() const noexcept { return _acquiredRelics; }
	ProgressionModifiers CalculateModifiers() const noexcept;

private:
	std::vector<std::string> BuildShuffledCycle();
	void MovePreferredOrbToFront();

	std::vector<std::string> _ownedOrbIds;
	std::vector<std::string> _currentCycle;
	std::vector<std::string> _nextCycle;
	std::size_t _cycleIndex = 0;
	std::size_t _reloadCount = 0;
	std::string _preferredOrbId = std::string(DefaultOrbId);
	std::mt19937 _shuffleGenerator;
	std::vector<std::string> _acquiredRelics;
};
