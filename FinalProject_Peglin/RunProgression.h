#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class RunStatus
{
	NotStarted,
	StageReady,
	RewardSelection,
	StageChoice,
	Complete,
	Defeated
};

enum class RunRewardKind
{
	Orb,
	Relic,
	Heal
};

struct RunReward
{
	RunRewardKind kind = RunRewardKind::Heal;
	std::string id;
	std::string displayName;
	float magnitude = 0.0f;
};

struct RunShopOffer
{
	RunReward reward;
	int price = 0;
};

inline constexpr std::string_view RunShopStageId = "goblin-market";
bool IsRunShopStage(std::string_view stageId) noexcept;
const std::array<RunShopOffer, 3>& GetRunShopOffers() noexcept;

struct RunStageEntry
{
	std::string id;
	bool isBoss = false;
};

using RunStageLayers = std::vector<std::vector<std::string>>;

struct AdventureRunSnapshot
{
	RunStatus status = RunStatus::NotStarted;
	RunStageLayers stageLayers;
	std::size_t currentLayer = 0;
	std::size_t completedCombatStages = 0;
	int gold = 0;
	std::string currentStageId;
	std::vector<std::string> clearedStageIds;
	std::optional<std::size_t> selectedStageChoiceIndex;
};

RunStageLayers BuildBranchingStageLayers(
	const std::vector<RunStageEntry>& catalogStages);

class AdventureRun
{
public:
	bool Start(std::vector<std::string> orderedStageIds);
	bool StartBranching(RunStageLayers stageLayers);
	bool CompleteCurrentStage(bool grantCombatReward = true);
	void MarkDefeated() noexcept;
	bool RetryCurrentStage() noexcept;
	std::optional<RunReward> SelectReward(std::size_t index);
	bool SelectNextStage(std::size_t index);
	bool ConfirmSelectedStage();
	bool SpendGold(int amount) noexcept;
	AdventureRunSnapshot CreateSnapshot() const;
	bool RestoreSnapshot(const AdventureRunSnapshot& snapshot);

	RunStatus GetStatus() const noexcept { return _status; }
	std::size_t GetStageCount() const noexcept { return _stageLayers.size(); }
	std::size_t GetClearedStageCount() const noexcept { return _clearedStageIds.size(); }
	std::size_t GetCompletedCombatStageCount() const noexcept { return _completedCombatStages; }
	int GetGold() const noexcept { return _gold; }
	std::size_t GetCurrentStageIndex() const noexcept;
	const std::string& GetCurrentStageId() const noexcept;
	const std::vector<RunReward>& GetRewardChoices() const noexcept { return _rewardChoices; }
	const std::vector<std::string>& GetAvailableStageIds() const noexcept { return _stageChoices; }
	std::optional<std::size_t> GetSelectedStageChoiceIndex() const noexcept
	{
		return _selectedStageChoiceIndex;
	}
	const std::string& GetSelectedStageChoiceId() const noexcept;
	const std::vector<std::string>& GetClearedStageIds() const noexcept { return _clearedStageIds; }
	bool HasClearedStage(std::string_view stageId) const noexcept;

private:
	void BuildRewardChoices();
	void BuildStageChoices();

	RunStatus _status = RunStatus::NotStarted;
	RunStageLayers _stageLayers;
	std::size_t _currentLayer = 0;
	std::size_t _completedCombatStages = 0;
	int _gold = 0;
	std::string _currentStageId;
	std::vector<std::string> _clearedStageIds;
	std::vector<std::string> _stageChoices;
	std::optional<std::size_t> _selectedStageChoiceIndex;
	std::vector<RunReward> _rewardChoices;
};
