#include "pch.h"
#include "UiNavigation.h"

#include <algorithm>

namespace
{
	struct UiRect
	{
		float left;
		float top;
		float right;
		float bottom;

		bool Contains(Vector2 position) const noexcept
		{
			return position.x >= left && position.x < right
				&& position.y >= top && position.y < bottom;
		}
	};

	UiAction ResolveStageSelectionClick(
		Vector2 position,
		std::size_t visibleStageCount) noexcept
	{
		const std::size_t safeStageCount = (std::min)(std::size_t{ 2 }, visibleStageCount);
		for (std::size_t index = 0; index < safeStageCount; ++index)
		{
			const float left = safeStageCount == 1
				? 437.0f
				: (index == 0 ? 252.0f : 620.0f);
			if (UiRect{ left, 215.0f, left + 343.0f, 590.0f }.Contains(position))
			{
				return { UiCommand::SelectStage, index };
			}
		}

		if (UiRect{ 350.0f, 635.0f, 865.0f, 695.0f }.Contains(position))
		{
			return { UiCommand::StartSelectedStage };
		}
		if (UiRect{ 43.0f, 565.0f, 206.0f, 602.0f }.Contains(position))
		{
			return { UiCommand::OpenStatistics };
		}
		if (UiRect{ 43.0f, 608.0f, 206.0f, 646.0f }.Contains(position))
		{
			return { UiCommand::OpenLoadout };
		}
		if (UiRect{ 43.0f, 652.0f, 206.0f, 690.0f }.Contains(position))
		{
			return { UiCommand::OpenOptions };
		}
		return {};
	}

	UiAction ResolveStatisticsClick(Vector2 position) noexcept
	{
		if (UiRect{ 80.0f, 110.0f, 300.0f, 160.0f }.Contains(position))
		{
			return { UiCommand::CycleStatisticsDifficulty };
		}
		if (UiRect{ 350.0f, 110.0f, 570.0f, 160.0f }.Contains(position))
		{
			return { UiCommand::CycleStatisticsSort };
		}
		if (UiRect{ 720.0f, 630.0f, 945.0f, 688.0f }.Contains(position))
		{
			return { UiCommand::BackToStageSelection };
		}
		return {};
	}

	UiAction ResolveLoadoutClick(Vector2 position) noexcept
	{
		for (std::size_t index = 0; index < 5; ++index)
		{
			const float left = 55.0f + static_cast<float>(index) * 180.0f;
			if (UiRect{ left, 148.0f, left + 170.0f, 320.0f }.Contains(position))
			{
				return { UiCommand::SelectOrb, index };
			}
			if (UiRect{ left, 360.0f, left + 170.0f, 545.0f }.Contains(position))
			{
				return { UiCommand::AcquireRelic, index };
			}
		}

		if (UiRect{ 55.0f, 630.0f, 300.0f, 688.0f }.Contains(position))
		{
			return { UiCommand::ResetProgression };
		}
		if (UiRect{ 700.0f, 630.0f, 945.0f, 688.0f }.Contains(position))
		{
			return { UiCommand::BackToStageSelection };
		}
		return {};
	}

	UiAction ResolveOptionsClick(Vector2 position) noexcept
	{
		static constexpr UiRect tiles[]{
			{185,125,390,190},{397,125,602,190},{609,125,815,190},
			{185,198,390,263},{397,198,602,263},{609,198,815,263},
			{185,271,390,336},{397,271,602,336},{609,271,815,336},
			{185,344,390,409},{397,344,602,409},{609,344,815,409}
		};
		static constexpr UiCommand commands[]{
			UiCommand::ToggleDifficulty, UiCommand::ToggleSound, UiCommand::CycleEffectsVolume,
			UiCommand::CycleMusicVolume, UiCommand::TogglePegColorMode, UiCommand::ToggleLanguage,
			UiCommand::CycleGamepadDeadzone, UiCommand::CycleGamepadSensitivity,
			UiCommand::ToggleGamepadFireBinding,
			UiCommand::CycleKeyboardPauseBinding,
			UiCommand::CycleKeyboardCombatLogBinding,
			UiCommand::ToggleMouseAimBinding
		};
		for (std::size_t index = 0; index < 12U; ++index)
		{
			if (tiles[index].Contains(position)) return { commands[index] };
		}
		if (UiRect{ 195.0f, 425.0f, 485.0f, 468.0f }.Contains(position))
		{
			return { UiCommand::ResetSettingsData };
		}
		if (UiRect{ 515.0f, 425.0f, 805.0f, 468.0f }.Contains(position))
		{
			return { UiCommand::ResetRecordData };
		}
		if (UiRect{ 300.0f, 575.0f, 700.0f, 632.0f }.Contains(position))
		{
			return { UiCommand::BackToStageSelection };
		}
		return {};
	}

	UiAction ResolveResultClick(Vector2 position) noexcept
	{
		if (UiRect{ 260.0f, 635.0f, 480.0f, 690.0f }.Contains(position))
		{
			return { UiCommand::RetryStage };
		}
		if (UiRect{ 520.0f, 635.0f, 740.0f, 690.0f }.Contains(position))
		{
			return { UiCommand::BackToStageSelection };
		}
		return {};
	}

	UiAction ResolveRewardClick(Vector2 position) noexcept
	{
		for (std::size_t index = 0; index < 3; ++index)
		{
			const float left = 60.0f + static_cast<float>(index) * 310.0f;
			if (UiRect{ left, 175.0f, left + 280.0f, 520.0f }.Contains(position))
			{
				return { UiCommand::SelectReward, index };
			}
		}
		return {};
	}

	UiAction ResolveShopClick(Vector2 position) noexcept
	{
		for (std::size_t index = 0; index < 3; ++index)
		{
			const float left = 290.0f + static_cast<float>(index) * 225.0f;
			if (UiRect{ left, 165.0f, left + 210.0f, 535.0f }.Contains(position))
			{
				return { UiCommand::BuyShopOffer, index };
			}
		}
		if (UiRect{ 340.0f, 630.0f, 660.0f, 688.0f }.Contains(position))
		{
			return { UiCommand::LeaveShop };
		}
		return {};
	}
}

UiAction ResolveUiClick(
	UiScreenKind screen,
	Vector2 position,
	std::size_t visibleStageCount) noexcept
{
	switch (screen)
	{
	case UiScreenKind::StageSelection:
		return ResolveStageSelectionClick(position, visibleStageCount);
	case UiScreenKind::Loadout:
		return ResolveLoadoutClick(position);
	case UiScreenKind::Options:
		return ResolveOptionsClick(position);
	case UiScreenKind::Statistics:
		return ResolveStatisticsClick(position);
	case UiScreenKind::Reward:
		return ResolveRewardClick(position);
	case UiScreenKind::Shop:
		return ResolveShopClick(position);
	case UiScreenKind::Result:
		return ResolveResultClick(position);
	}
	return {};
}
