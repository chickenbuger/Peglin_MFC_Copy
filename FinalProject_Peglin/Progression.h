#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class RelicDuplicatePolicy
{
	Unique,
	Stackable
};

struct OrbDefinition
{
	std::string_view id;
	std::string_view displayName;
	float pegDamageMultiplier = 1.0f;
	float scoreMultiplier = 1.0f;
};

struct RelicDefinition
{
	std::string_view id;
	std::string_view displayName;
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

const std::vector<OrbDefinition>& GetOrbDefinitions() noexcept;
const std::vector<RelicDefinition>& GetRelicDefinitions() noexcept;
const OrbDefinition* FindOrbDefinition(std::string_view id) noexcept;
const RelicDefinition* FindRelicDefinition(std::string_view id) noexcept;

class PlayerLoadout
{
public:
	static constexpr std::string_view DefaultOrbId = "basic-orb";

	bool SelectOrb(std::string_view id);
	bool AcquireRelic(std::string_view id);
	void Reset();

	const OrbDefinition& GetSelectedOrb() const noexcept;
	std::string_view GetSelectedOrbId() const noexcept { return _selectedOrbId; }
	std::size_t GetRelicStackCount(std::string_view id) const noexcept;
	const std::vector<std::string>& GetAcquiredRelics() const noexcept { return _acquiredRelics; }
	ProgressionModifiers CalculateModifiers() const noexcept;

private:
	std::string _selectedOrbId = std::string(DefaultOrbId);
	std::vector<std::string> _acquiredRelics;
};
