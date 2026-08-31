#include "pch.h"
#include "RunProgression.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

RunStageLayers BuildBranchingStageLayers(
	const std::vector<RunStageEntry>& catalogStages)
{
	if (catalogStages.size() < 3 || catalogStages.size() > 32)
	{
		return {};
	}

	std::vector<std::string> regularStages;
	std::string bossStage;
	std::unordered_set<std::string> ids;
	for (const RunStageEntry& entry : catalogStages)
	{
		if (entry.id.empty() || !ids.insert(entry.id).second)
		{
			return {};
		}
		if (entry.isBoss)
		{
			if (!bossStage.empty())
			{
				return {};
			}
			bossStage = entry.id;
		}
		else
		{
			regularStages.push_back(entry.id);
		}
	}
	if (regularStages.size() < 2 || bossStage.empty())
	{
		return {};
	}

	RunStageLayers layers;
	layers.push_back({ regularStages.front() });
	for (std::size_t index = 1; index < regularStages.size(); index += 2)
	{
		std::vector<std::string> layer{ regularStages[index] };
		if (index + 1 < regularStages.size())
		{
			layer.push_back(regularStages[index + 1]);
		}
		layers.push_back(std::move(layer));
	}
	layers.push_back({ std::move(bossStage) });
	return layers;
}

bool AdventureRun::Start(std::vector<std::string> orderedStageIds)
{
	RunStageLayers layers;
	layers.reserve(orderedStageIds.size());
	for (std::string& stageId : orderedStageIds)
	{
		layers.push_back({ std::move(stageId) });
	}
	return StartBranching(std::move(layers));
}

bool AdventureRun::StartBranching(RunStageLayers stageLayers)
{
	if (stageLayers.size() < 2
		|| stageLayers.front().size() != 1
		|| stageLayers.back().size() != 1)
	{
		return false;
	}
	std::unordered_set<std::string> ids;
	for (const std::vector<std::string>& layer : stageLayers)
	{
		if (layer.empty() || layer.size() > 2)
		{
			return false;
		}
		for (const std::string& id : layer)
		{
			if (id.empty() || !ids.insert(id).second)
			{
				return false;
			}
		}
	}

	_stageLayers = std::move(stageLayers);
	_currentLayer = 0;
	_currentStageId = _stageLayers.front().front();
	_clearedStageIds.clear();
	_stageChoices.clear();
	_rewardChoices.clear();
	_status = RunStatus::StageReady;
	return true;
}

bool AdventureRun::CompleteCurrentStage()
{
	if (_status != RunStatus::StageReady || _stageLayers.empty())
	{
		return false;
	}

	_clearedStageIds.push_back(_currentStageId);
	if (_currentLayer + 1 >= _stageLayers.size())
	{
		_status = RunStatus::Complete;
		_rewardChoices.clear();
		_stageChoices.clear();
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
	if (_status != RunStatus::Defeated || _stageLayers.empty())
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
	BuildStageChoices();
	_status = RunStatus::StageChoice;
	return selected;
}

bool AdventureRun::SelectNextStage(std::size_t index)
{
	if (_status != RunStatus::StageChoice || index >= _stageChoices.size())
	{
		return false;
	}

	_currentStageId = _stageChoices[index];
	++_currentLayer;
	_stageChoices.clear();
	_status = RunStatus::StageReady;
	return true;
}

std::size_t AdventureRun::GetCurrentStageIndex() const noexcept
{
	if (_stageLayers.empty())
	{
		return 0;
	}
	return (std::min)(_currentLayer, _stageLayers.size() - 1);
}

const std::string& AdventureRun::GetCurrentStageId() const noexcept
{
	static const std::string EMPTY_ID;
	return _stageLayers.empty() ? EMPTY_ID : _currentStageId;
}

bool AdventureRun::HasClearedStage(std::string_view stageId) const noexcept
{
	return std::any_of(
		_clearedStageIds.begin(),
		_clearedStageIds.end(),
		[stageId](const std::string& clearedId) { return clearedId == stageId; });
}

void AdventureRun::BuildRewardChoices()
{
	_rewardChoices.clear();
	if (_clearedStageIds.size() % 2 == 1)
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

void AdventureRun::BuildStageChoices()
{
	_stageChoices.clear();
	if (_currentLayer + 1 < _stageLayers.size())
	{
		_stageChoices = _stageLayers[_currentLayer + 1];
	}
}
