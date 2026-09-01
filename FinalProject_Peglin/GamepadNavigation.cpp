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
	case UiScreenKind::StageSelection: return SafeStageCount(visibleStageCount) + 3U;
	case UiScreenKind::Loadout: return 8U;
	case UiScreenKind::Options: return 7U;
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
		if (focusIndex == stageCount + 1U) return { UiCommand::OpenLoadout };
		if (focusIndex == stageCount + 2U) return { UiCommand::OpenOptions };
		break;
	case UiScreenKind::Loadout:
		if (focusIndex < 3U) return { UiCommand::SelectOrb, focusIndex };
		if (focusIndex < 6U) return { UiCommand::AcquireRelic, focusIndex - 3U };
		if (focusIndex == 6U) return { UiCommand::ResetProgression };
		if (focusIndex == 7U) return { UiCommand::BackToStageSelection };
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
		case 6U: return { UiCommand::BackToStageSelection };
		default: break;
		}
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
		if (focusIndex == stageCount + 1U) return { 43.0f, 575.0f, 206.0f, 625.0f };
		if (focusIndex == stageCount + 2U) return { 43.0f, 638.0f, 206.0f, 688.0f };
		break;
	case UiScreenKind::Loadout:
		if (focusIndex < 3U)
		{
			const float left = 55.0f + static_cast<float>(focusIndex) * 305.0f;
			return { left, 148.0f, left + 280.0f, 320.0f };
		}
		if (focusIndex < 6U)
		{
			const float left = 55.0f + static_cast<float>(focusIndex - 3U) * 305.0f;
			return { left, 360.0f, left + 280.0f, 545.0f };
		}
		if (focusIndex == 6U) return { 55.0f, 630.0f, 300.0f, 688.0f };
		if (focusIndex == 7U) return { 700.0f, 630.0f, 945.0f, 688.0f };
		break;
	case UiScreenKind::Options:
	{
		static constexpr UiFocusRect rects[]{
			{195,145,485,235},{515,145,805,235},{195,255,485,345},
			{515,255,805,345},{195,365,485,455},{515,365,805,455},{300,565,700,620}
		};
		if (focusIndex < 7U) return rects[focusIndex];
		break;
	}
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
