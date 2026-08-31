#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

enum class RunStatus
{
	NotStarted,
	StageReady,
	RewardSelection,
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

class AdventureRun
{
public:
	bool Start(std::vector<std::string> orderedStageIds);
	bool CompleteCurrentStage();
	void MarkDefeated() noexcept;
	bool RetryCurrentStage() noexcept;
	std::optional<RunReward> SelectReward(std::size_t index);

	RunStatus GetStatus() const noexcept { return _status; }
	std::size_t GetStageCount() const noexcept { return _stageIds.size(); }
	std::size_t GetClearedStageCount() const noexcept { return _clearedStages; }
	std::size_t GetCurrentStageIndex() const noexcept;
	const std::string& GetCurrentStageId() const noexcept;
	const std::vector<RunReward>& GetRewardChoices() const noexcept { return _rewardChoices; }

private:
	void BuildRewardChoices();

	RunStatus _status = RunStatus::NotStarted;
	std::vector<std::string> _stageIds;
	std::size_t _clearedStages = 0;
	std::vector<RunReward> _rewardChoices;
};
