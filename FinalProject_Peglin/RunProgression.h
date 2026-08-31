#pragma once

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

struct RunStageEntry
{
	std::string id;
	bool isBoss = false;
};

using RunStageLayers = std::vector<std::vector<std::string>>;

RunStageLayers BuildBranchingStageLayers(
	const std::vector<RunStageEntry>& catalogStages);

class AdventureRun
{
public:
	bool Start(std::vector<std::string> orderedStageIds);
	bool StartBranching(RunStageLayers stageLayers);
	bool CompleteCurrentStage();
	void MarkDefeated() noexcept;
	bool RetryCurrentStage() noexcept;
	std::optional<RunReward> SelectReward(std::size_t index);
	bool SelectNextStage(std::size_t index);

	RunStatus GetStatus() const noexcept { return _status; }
	std::size_t GetStageCount() const noexcept { return _stageLayers.size(); }
	std::size_t GetClearedStageCount() const noexcept { return _clearedStageIds.size(); }
	std::size_t GetCurrentStageIndex() const noexcept;
	const std::string& GetCurrentStageId() const noexcept;
	const std::vector<RunReward>& GetRewardChoices() const noexcept { return _rewardChoices; }
	const std::vector<std::string>& GetAvailableStageIds() const noexcept { return _stageChoices; }
	const std::vector<std::string>& GetClearedStageIds() const noexcept { return _clearedStageIds; }
	bool HasClearedStage(std::string_view stageId) const noexcept;

private:
	void BuildRewardChoices();
	void BuildStageChoices();

	RunStatus _status = RunStatus::NotStarted;
	RunStageLayers _stageLayers;
	std::size_t _currentLayer = 0;
	std::string _currentStageId;
	std::vector<std::string> _clearedStageIds;
	std::vector<std::string> _stageChoices;
	std::vector<RunReward> _rewardChoices;
};
