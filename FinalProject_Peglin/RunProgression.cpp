#include "pch.h"
#include "RunProgression.h"

#include <algorithm>
#include <utility>

bool AdventureRun::Start(std::vector<std::string> orderedStageIds)
{
	if (orderedStageIds.empty()
		|| std::any_of(
			orderedStageIds.begin(),
			orderedStageIds.end(),
			[](const std::string& id) { return id.empty(); }))
	{
		return false;
	}

	_stageIds = std::move(orderedStageIds);
	_clearedStages = 0;
	_rewardChoices.clear();
	_status = RunStatus::StageReady;
	return true;
}

bool AdventureRun::CompleteCurrentStage()
{
	if (_status != RunStatus::StageReady || _stageIds.empty())
	{
		return false;
	}

	++_clearedStages;
	if (_clearedStages >= _stageIds.size())
	{
		_status = RunStatus::Complete;
		_rewardChoices.clear();
		return true;
	}

	BuildRewardChoices();
	_status = RunStatus::RewardSelection;
	return true;
}

void AdventureRun::MarkDefeated() noexcept
{
	if (_status == RunStatus::StageReady)
	{
		_status = RunStatus::Defeated;
	}
}

bool AdventureRun::RetryCurrentStage() noexcept
{
	if (_status != RunStatus::Defeated || _stageIds.empty())
	{
		return false;
	}
	_status = RunStatus::StageReady;
	return true;
}

std::optional<RunReward> AdventureRun::SelectReward(std::size_t index)
{
	if (_status != RunStatus::RewardSelection || index >= _rewardChoices.size())
	{
		return std::nullopt;
	}

	RunReward selected = _rewardChoices[index];
	_rewardChoices.clear();
	_status = RunStatus::StageReady;
	return selected;
}

std::size_t AdventureRun::GetCurrentStageIndex() const noexcept
{
	if (_stageIds.empty())
	{
		return 0;
	}
	return (std::min)(_clearedStages, _stageIds.size() - 1);
}

const std::string& AdventureRun::GetCurrentStageId() const noexcept
{
	static const std::string EMPTY_ID;
	return _stageIds.empty() ? EMPTY_ID : _stageIds[GetCurrentStageIndex()];
}

void AdventureRun::BuildRewardChoices()
{
	_rewardChoices.clear();
	if (_clearedStages == 1)
	{
		_rewardChoices = {
			{ RunRewardKind::Orb, "iron-orb", "Iron Orb", 0.0f },
			{ RunRewardKind::Relic, "combo-lantern", "Combo Lantern", 0.0f },
			{ RunRewardKind::Heal, {}, "Heal 25 HP", 25.0f }
		};
	}
	else
	{
		_rewardChoices = {
			{ RunRewardKind::Orb, "echo-orb", "Echo Orb", 0.0f },
			{ RunRewardKind::Relic, "thorn-charm", "Thorn Charm", 0.0f },
			{ RunRewardKind::Heal, {}, "Heal 35 HP", 35.0f }
		};
	}
}
