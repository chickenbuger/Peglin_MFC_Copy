#pragma once

#include "Vector2.h"

#include <cstddef>

enum class UiScreenKind
{
	StageSelection,
	Loadout,
	Options,
	Statistics,
	Reward,
	Shop,
	Result
};

enum class UiCommand
{
	None,
	SelectStage,
	StartSelectedStage,
	OpenLoadout,
	OpenOptions,
	OpenStatistics,
	SelectOrb,
	AcquireRelic,
	ResetProgression,
	BackToStageSelection,
	ToggleDifficulty,
	ToggleSound,
	CycleEffectsVolume,
	CycleMusicVolume,
	TogglePegColorMode,
	ToggleLanguage,
	CycleGamepadDeadzone,
	CycleGamepadSensitivity,
	ToggleGamepadFireBinding,
	ResetSettingsData,
	ResetRecordData,
	CycleStatisticsDifficulty,
	CycleStatisticsSort,
	SelectReward,
	BuyShopOffer,
	LeaveShop,
	RetryStage
};

struct UiAction
{
	UiCommand command = UiCommand::None;
	std::size_t index = 0;

	bool IsHandled() const noexcept { return command != UiCommand::None; }
};

UiAction ResolveUiClick(
	UiScreenKind screen,
	Vector2 position,
	std::size_t visibleStageCount) noexcept;
