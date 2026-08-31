#pragma once

#include "Vector2.h"

#include <cstddef>

enum class UiScreenKind
{
	StageSelection,
	Loadout,
	Options,
	Reward,
	Result
};

enum class UiCommand
{
	None,
	SelectStage,
	StartSelectedStage,
	OpenLoadout,
	OpenOptions,
	SelectOrb,
	AcquireRelic,
	ResetProgression,
	BackToStageSelection,
	ToggleDifficulty,
	ToggleSound,
	TogglePegColorMode,
	SelectReward,
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
