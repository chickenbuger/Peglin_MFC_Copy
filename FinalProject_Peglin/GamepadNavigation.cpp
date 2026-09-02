#include "pch.h"
#include "GamepadNavigation.h"

#include <algorithm>

namespace
{
	std::size_t SafeStageCount(std::size_t count) noexcept
	{
		return (std::min)(std::size_t{ 2 }, count);
	}
}

std::size_t GetGamepadFocusCount(UiScreenKind screen, std::size_t visibleStageCount) noexcept
{
	switch (screen)
	{
	case UiScreenKind::StageSelection: return SafeStageCount(visibleStageCount) + 4U;
	case UiScreenKind::Loadout: return 12U;
	case UiScreenKind::Options: return 15U;
	case UiScreenKind::Statistics: return 3U;
	case UiScreenKind::Reward: return 3U;
	case UiScreenKind::Shop: return 4U;
	case UiScreenKind::Result: return 2U;
	}
	return 0U;
}

std::size_t MoveGamepadFocus(std::size_t currentIndex, std::size_t focusCount, int direction) noexcept
{
	if (focusCount == 0U)
	{
		return 0U;
	}
	currentIndex %= focusCount;
	if (direction > 0)
	{
		return (currentIndex + 1U) % focusCount;
	}
	if (direction < 0)
	{
		return currentIndex == 0U ? focusCount - 1U : currentIndex - 1U;
	}
	return currentIndex;
}

UiAction ResolveGamepadFocusedAction(
	UiScreenKind screen,
	std::size_t focusIndex,
	std::size_t visibleStageCount) noexcept
{
	const std::size_t stageCount = SafeStageCount(visibleStageCount);
	switch (screen)
	{
	case UiScreenKind::StageSelection:
		if (focusIndex < stageCount) return { UiCommand::SelectStage, focusIndex };
		if (focusIndex == stageCount) return { UiCommand::StartSelectedStage };
		if (focusIndex == stageCount + 1U) return { UiCommand::OpenStatistics };
		if (focusIndex == stageCount + 2U) return { UiCommand::OpenLoadout };
		if (focusIndex == stageCount + 3U) return { UiCommand::OpenOptions };
		break;
	case UiScreenKind::Loadout:
		if (focusIndex < 5U) return { UiCommand::SelectOrb, focusIndex };
		if (focusIndex < 10U) return { UiCommand::AcquireRelic, focusIndex - 5U };
		if (focusIndex == 10U) return { UiCommand::ResetProgression };
		if (focusIndex == 11U) return { UiCommand::BackToStageSelection };
		break;
	case UiScreenKind::Options:
		switch (focusIndex)
		{
		case 0U: return { UiCommand::ToggleDifficulty };
		case 1U: return { UiCommand::ToggleSound };
		case 2U: return { UiCommand::CycleEffectsVolume };
		case 3U: return { UiCommand::CycleMusicVolume };
		case 4U: return { UiCommand::TogglePegColorMode };
		case 5U: return { UiCommand::ToggleLanguage };
		case 6U: return { UiCommand::CycleGamepadDeadzone };
		case 7U: return { UiCommand::CycleGamepadSensitivity };
		case 8U: return { UiCommand::ToggleGamepadFireBinding };
		case 9U: return { UiCommand::CycleKeyboardPauseBinding };
		case 10U: return { UiCommand::CycleKeyboardCombatLogBinding };
		case 11U: return { UiCommand::ToggleMouseAimBinding };
		case 12U: return { UiCommand::ResetSettingsData };
		case 13U: return { UiCommand::ResetRecordData };
		case 14U: return { UiCommand::BackToStageSelection };
		default: break;
		}
		break;
	case UiScreenKind::Statistics:
		if (focusIndex == 0U) return { UiCommand::CycleStatisticsDifficulty };
		if (focusIndex == 1U) return { UiCommand::CycleStatisticsSort };
		if (focusIndex == 2U) return { UiCommand::BackToStageSelection };
		break;
	case UiScreenKind::Reward:
		if (focusIndex < 3U) return { UiCommand::SelectReward, focusIndex };
		break;
	case UiScreenKind::Shop:
		if (focusIndex < 3U) return { UiCommand::BuyShopOffer, focusIndex };
		if (focusIndex == 3U) return { UiCommand::LeaveShop };
		break;
	case UiScreenKind::Result:
		if (focusIndex == 0U) return { UiCommand::RetryStage };
		if (focusIndex == 1U) return { UiCommand::BackToStageSelection };
		break;
	}
	return {};
}

UiAction ResolveGamepadBackAction(UiScreenKind screen) noexcept
{
	switch (screen)
	{
	case UiScreenKind::Loadout:
	case UiScreenKind::Options:
	case UiScreenKind::Statistics:
	case UiScreenKind::Result:
		return { UiCommand::BackToStageSelection };
	case UiScreenKind::Shop:
		return { UiCommand::LeaveShop };
	case UiScreenKind::StageSelection:
	case UiScreenKind::Reward:
		return {};
	}
	return {};
}

UiFocusRect GetGamepadFocusRect(
	UiScreenKind screen,
	std::size_t focusIndex,
	std::size_t visibleStageCount) noexcept
{
	const std::size_t stageCount = SafeStageCount(visibleStageCount);
	switch (screen)
	{
	case UiScreenKind::StageSelection:
		if (focusIndex < stageCount)
		{
			const float left = stageCount == 1U ? 437.0f : (focusIndex == 0U ? 252.0f : 620.0f);
			return { left, 215.0f, left + 343.0f, 590.0f };
		}
		if (focusIndex == stageCount) return { 350.0f, 635.0f, 865.0f, 695.0f };
		if (focusIndex == stageCount + 1U) return { 43.0f, 565.0f, 206.0f, 602.0f };
		if (focusIndex == stageCount + 2U) return { 43.0f, 608.0f, 206.0f, 646.0f };
		if (focusIndex == stageCount + 3U) return { 43.0f, 652.0f, 206.0f, 690.0f };
		break;
	case UiScreenKind::Loadout:
		if (focusIndex < 5U)
		{
			const float left = 55.0f + static_cast<float>(focusIndex) * 180.0f;
			return { left, 148.0f, left + 170.0f, 320.0f };
		}
		if (focusIndex < 10U)
		{
			const float left = 55.0f + static_cast<float>(focusIndex - 5U) * 180.0f;
			return { left, 360.0f, left + 170.0f, 545.0f };
		}
		if (focusIndex == 10U) return { 55.0f, 630.0f, 300.0f, 688.0f };
		if (focusIndex == 11U) return { 700.0f, 630.0f, 945.0f, 688.0f };
		break;
	case UiScreenKind::Options:
	{
		static constexpr UiFocusRect rects[]{
			{185,125,390,190},{397,125,602,190},{609,125,815,190},
			{185,198,390,263},{397,198,602,263},{609,198,815,263},
			{185,271,390,336},{397,271,602,336},{609,271,815,336},
			{185,344,390,409},{397,344,602,409},{609,344,815,409},
			{195,425,485,468},{515,425,805,468},{300,575,700,632}
		};
		if (focusIndex < 15U) return rects[focusIndex];
		break;
	}
	case UiScreenKind::Statistics:
		if (focusIndex == 0U) return { 80.0f, 110.0f, 300.0f, 160.0f };
		if (focusIndex == 1U) return { 350.0f, 110.0f, 570.0f, 160.0f };
		if (focusIndex == 2U) return { 720.0f, 630.0f, 945.0f, 688.0f };
		break;
	case UiScreenKind::Reward:
		if (focusIndex < 3U)
		{
			const float left = 60.0f + static_cast<float>(focusIndex) * 310.0f;
			return { left, 175.0f, left + 280.0f, 520.0f };
		}
		break;
	case UiScreenKind::Shop:
		if (focusIndex < 3U)
		{
			const float left = 290.0f + static_cast<float>(focusIndex) * 225.0f;
			return { left, 165.0f, left + 210.0f, 535.0f };
		}
		if (focusIndex == 3U) return { 340.0f, 630.0f, 660.0f, 688.0f };
		break;
	case UiScreenKind::Result:
		if (focusIndex == 0U) return { 260.0f, 635.0f, 480.0f, 690.0f };
		if (focusIndex == 1U) return { 520.0f, 635.0f, 740.0f, 690.0f };
		break;
	}
	return {};
}

Vector2 ApplyGamepadStickTuning(
	Vector2 rawStick,
	int deadzonePercent,
	int sensitivityPercent) noexcept
{
	const float deadzone = std::clamp(
		static_cast<float>(deadzonePercent) / 100.0f,
		0.05f,
		0.60f);
	const float sensitivity = std::clamp(
		static_cast<float>(sensitivityPercent) / 100.0f,
		0.50f,
		2.00f);
	const float rawLength = (std::min)(rawStick.Length(), 1.0f);
	if (rawLength <= deadzone)
	{
		return {};
	}
	const float normalizedMagnitude = std::clamp(
		((rawLength - deadzone) / (1.0f - deadzone)) * sensitivity,
		0.0f,
		1.0f);
	return rawStick.Normalized() * normalizedMagnitude;
}

bool ShouldFireGamepadShot(
	GamepadFireBinding binding,
	bool southButtonPressed,
	bool rightTriggerPressed) noexcept
{
	return binding == GamepadFireBinding::SouthButton
		? southButtonPressed
		: rightTriggerPressed;
}
