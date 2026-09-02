#include "pch.h"
#include "RunProgression.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace
{
	constexpr int STARTING_GOLD = 50;
	constexpr int COMBAT_CLEAR_GOLD = 25;
	constexpr int MAX_RUN_GOLD = 999999;

	const std::array<RunShopOffer, 3> SHOP_OFFERS{
		RunShopOffer{ { RunRewardKind::Orb, "iron-orb", "Iron Orb", 0.0f }, 45 },
		RunShopOffer{ { RunRewardKind::Relic, "bark-guard", "Bark Guard", 0.0f }, 70 },
		RunShopOffer{ { RunRewardKind::Heal, {}, "Heal 30 HP", 30.0f }, 30 }
	};

	bool IsSafeRunId(std::string_view id) noexcept
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
}

bool IsRunShopStage(std::string_view stageId) noexcept
{
	return stageId == RunShopStageId;
}

const std::array<RunShopOffer, 3>& GetRunShopOffers() noexcept
{
	return SHOP_OFFERS;
}

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
		if (entry.id.empty()
			|| IsRunShopStage(entry.id)
			|| !ids.insert(entry.id).second)
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
	const std::size_t shopLayerIndex = (std::min)(std::size_t{ 3 }, layers.size());
	layers.insert(layers.begin() + static_cast<std::ptrdiff_t>(shopLayerIndex),
		{ std::string(RunShopStageId) });
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
	_completedCombatStages = 0;
	_gold = STARTING_GOLD;
	_currentStageId = _stageLayers.front().front();
	_clearedStageIds.clear();
	_stageChoices.clear();
	_selectedStageChoiceIndex.reset();
	_rewardChoices.clear();
	_status = RunStatus::StageReady;
	return true;
}

bool AdventureRun::CompleteCurrentStage(bool grantCombatReward)
{
	if (_status != RunStatus::StageReady || _stageLayers.empty())
	{
		return false;
	}
	const bool shopStage = IsRunShopStage(_currentStageId);
	if (shopStage == grantCombatReward)
	{
		return false;
	}

	_clearedStageIds.push_back(_currentStageId);
	if (grantCombatReward)
	{
		++_completedCombatStages;
		_gold += COMBAT_CLEAR_GOLD;
	}
	if (_currentLayer + 1 >= _stageLayers.size())
	{
		_status = RunStatus::Complete;
		_rewardChoices.clear();
		_stageChoices.clear();
		_selectedStageChoiceIndex.reset();
		return true;
	}

	if (grantCombatReward)
	{
		BuildRewardChoices();
		_status = RunStatus::RewardSelection;
	}
	else
	{
		BuildStageChoices();
		_status = RunStatus::StageChoice;
	}
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

	_selectedStageChoiceIndex = index;
	return true;
}

bool AdventureRun::ConfirmSelectedStage()
{
	if (_status != RunStatus::StageChoice
		|| !_selectedStageChoiceIndex.has_value()
		|| *_selectedStageChoiceIndex >= _stageChoices.size())
	{
		return false;
	}

	_currentStageId = _stageChoices[*_selectedStageChoiceIndex];
	++_currentLayer;
	_stageChoices.clear();
	_selectedStageChoiceIndex.reset();
	_status = RunStatus::StageReady;
	return true;
}

bool AdventureRun::SpendGold(int amount) noexcept
{
	if (amount <= 0 || amount > _gold)
	{
		return false;
	}
	_gold -= amount;
	return true;
}

AdventureRunSnapshot AdventureRun::CreateSnapshot() const
{
	AdventureRunSnapshot snapshot;
	snapshot.status = _status;
	snapshot.stageLayers = _stageLayers;
	snapshot.currentLayer = _currentLayer;
	snapshot.completedCombatStages = _completedCombatStages;
	snapshot.gold = _gold;
	snapshot.currentStageId = _currentStageId;
	snapshot.clearedStageIds = _clearedStageIds;
	snapshot.selectedStageChoiceIndex = _selectedStageChoiceIndex;
	return snapshot;
}

bool AdventureRun::RestoreSnapshot(const AdventureRunSnapshot& snapshot)
{
	if (snapshot.status == RunStatus::NotStarted
		|| snapshot.stageLayers.size() < 2
		|| snapshot.stageLayers.size() > 32
		|| snapshot.stageLayers.front().size() != 1
		|| snapshot.stageLayers.back().size() != 1
		|| snapshot.currentLayer >= snapshot.stageLayers.size()
		|| snapshot.gold < 0
		|| snapshot.gold > MAX_RUN_GOLD)
	{
		return false;
	}

	std::unordered_set<std::string> routeIds;
	for (const std::vector<std::string>& layer : snapshot.stageLayers)
	{
		if (layer.empty() || layer.size() > 2)
		{
			return false;
		}
		for (const std::string& id : layer)
		{
			if (!IsSafeRunId(id) || !routeIds.insert(id).second)
			{
				return false;
			}
		}
	}

	const std::vector<std::string>& currentLayer =
		snapshot.stageLayers[snapshot.currentLayer];
	if (std::find(currentLayer.begin(), currentLayer.end(), snapshot.currentStageId)
		== currentLayer.end())
	{
		return false;
	}

	std::size_t expectedCleared = snapshot.currentLayer;
	if (snapshot.status == RunStatus::RewardSelection
		|| snapshot.status == RunStatus::StageChoice)
	{
		expectedCleared = snapshot.currentLayer + 1;
	}
	else if (snapshot.status == RunStatus::Complete)
	{
		expectedCleared = snapshot.stageLayers.size();
	}
	else if (snapshot.status != RunStatus::StageReady
		&& snapshot.status != RunStatus::Defeated)
	{
		return false;
	}
	if (snapshot.clearedStageIds.size() != expectedCleared)
	{
		return false;
	}

	std::size_t combatClears = 0;
	for (std::size_t index = 0; index < snapshot.clearedStageIds.size(); ++index)
	{
		const std::string& clearedId = snapshot.clearedStageIds[index];
		const std::vector<std::string>& layer = snapshot.stageLayers[index];
		if (std::find(layer.begin(), layer.end(), clearedId) == layer.end())
		{
			return false;
		}
		if (!IsRunShopStage(clearedId))
		{
			++combatClears;
		}
	}
	if (combatClears != snapshot.completedCombatStages)
	{
		return false;
	}
	if ((snapshot.status == RunStatus::RewardSelection
			|| snapshot.status == RunStatus::StageChoice
			|| snapshot.status == RunStatus::Complete)
		&& snapshot.clearedStageIds.back() != snapshot.currentStageId)
	{
		return false;
	}
	if (snapshot.status == RunStatus::StageChoice)
	{
		if (snapshot.currentLayer + 1 >= snapshot.stageLayers.size())
		{
			return false;
		}
		const std::size_t choiceCount = snapshot.stageLayers[snapshot.currentLayer + 1].size();
		if (snapshot.selectedStageChoiceIndex.has_value()
			&& *snapshot.selectedStageChoiceIndex >= choiceCount)
		{
			return false;
		}
	}
	else if (snapshot.selectedStageChoiceIndex.has_value())
	{
		return false;
	}

	_status = snapshot.status;
	_stageLayers = snapshot.stageLayers;
	_currentLayer = snapshot.currentLayer;
	_completedCombatStages = snapshot.completedCombatStages;
	_gold = snapshot.gold;
	_currentStageId = snapshot.currentStageId;
	_clearedStageIds = snapshot.clearedStageIds;
	_stageChoices.clear();
	_selectedStageChoiceIndex.reset();
	_rewardChoices.clear();
	if (_status == RunStatus::RewardSelection)
	{
		BuildRewardChoices();
	}
	else if (_status == RunStatus::StageChoice)
	{
		BuildStageChoices();
		_selectedStageChoiceIndex = snapshot.selectedStageChoiceIndex;
	}
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

const std::string& AdventureRun::GetSelectedStageChoiceId() const noexcept
{
	static const std::string EMPTY_ID;
	if (_status != RunStatus::StageChoice
		|| !_selectedStageChoiceIndex.has_value()
		|| *_selectedStageChoiceIndex >= _stageChoices.size())
	{
		return EMPTY_ID;
	}
	return _stageChoices[*_selectedStageChoiceIndex];
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
	switch (_completedCombatStages % 4)
	{
	case 1:
		_rewardChoices = {
			{ RunRewardKind::Orb, "iron-orb", "Iron Orb", 0.0f },
			{ RunRewardKind::Relic, "combo-lantern", "Combo Lantern", 0.0f },
			{ RunRewardKind::Heal, {}, "Heal 25 HP", 25.0f }
		};
		break;
	case 2:
		_rewardChoices = {
			{ RunRewardKind::Orb, "echo-orb", "Echo Orb", 0.0f },
			{ RunRewardKind::Relic, "thorn-charm", "Thorn Charm", 0.0f },
			{ RunRewardKind::Heal, {}, "Heal 35 HP", 35.0f }
		};
		break;
	case 3:
		_rewardChoices = {
			{ RunRewardKind::Orb, "cinder-orb", "Cinder Orb", 0.0f },
			{ RunRewardKind::Relic, "ember-heart", "Ember Heart", 0.0f },
			{ RunRewardKind::Heal, {}, "Heal 30 HP", 30.0f }
		};
		break;
	default:
		_rewardChoices = {
			{ RunRewardKind::Orb, "verdant-orb", "Verdant Orb", 0.0f },
			{ RunRewardKind::Relic, "wayfinder-compass", "Wayfinder Compass", 0.0f },
			{ RunRewardKind::Heal, {}, "Heal 40 HP", 40.0f }
		};
		break;
	}
}

void AdventureRun::BuildStageChoices()
{
	_stageChoices.clear();
	_selectedStageChoiceIndex.reset();
	if (_currentLayer + 1 < _stageLayers.size())
	{
		_stageChoices = _stageLayers[_currentLayer + 1];
	}
}
