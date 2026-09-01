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
		if (UiRect{ 43.0f, 575.0f, 206.0f, 625.0f }.Contains(position))
		{
			return { UiCommand::OpenLoadout };
		}
		if (UiRect{ 43.0f, 638.0f, 206.0f, 688.0f }.Contains(position))
		{
			return { UiCommand::OpenOptions };
		}
		return {};
	}

	UiAction ResolveLoadoutClick(Vector2 position) noexcept
	{
		for (std::size_t index = 0; index < 3; ++index)
		{
			const float left = 75.0f + static_cast<float>(index) * 305.0f;
			if (UiRect{ left, 185.0f, left + 270.0f, 335.0f }.Contains(position))
			{
				return { UiCommand::SelectOrb, index };
			}
			if (UiRect{ left, 390.0f, left + 270.0f, 555.0f }.Contains(position))
			{
				return { UiCommand::AcquireRelic, index };
			}
		}

		if (UiRect{ 75.0f, 640.0f, 300.0f, 690.0f }.Contains(position))
		{
			return { UiCommand::ResetProgression };
		}
		if (UiRect{ 680.0f, 640.0f, 905.0f, 690.0f }.Contains(position))
		{
			return { UiCommand::BackToStageSelection };
		}
		return {};
	}

	UiAction ResolveOptionsClick(Vector2 position) noexcept
	{
		if (UiRect{ 285.0f, 205.0f, 695.0f, 285.0f }.Contains(position))
		{
			return { UiCommand::ToggleDifficulty };
		}
		if (UiRect{ 285.0f, 315.0f, 695.0f, 395.0f }.Contains(position))
		{
			return { UiCommand::ToggleSound };
		}
		if (UiRect{ 285.0f, 425.0f, 695.0f, 505.0f }.Contains(position))
		{
			return { UiCommand::TogglePegColorMode };
		}
		if (UiRect{ 285.0f, 520.0f, 695.0f, 600.0f }.Contains(position))
		{
			return { UiCommand::ToggleLanguage };
		}
		if (UiRect{ 300.0f, 640.0f, 680.0f, 690.0f }.Contains(position))
		{
			return { UiCommand::BackToStageSelection };
		}
		return {};
	}

	UiAction ResolveResultClick(Vector2 position) noexcept
	{
		if (UiRect{ 260.0f, 640.0f, 480.0f, 690.0f }.Contains(position))
		{
			return { UiCommand::RetryStage };
		}
		if (UiRect{ 500.0f, 640.0f, 720.0f, 690.0f }.Contains(position))
		{
			return { UiCommand::BackToStageSelection };
		}
		return {};
	}

	UiAction ResolveRewardClick(Vector2 position) noexcept
	{
		for (std::size_t index = 0; index < 3; ++index)
		{
			const float left = 75.0f + static_cast<float>(index) * 305.0f;
			if (UiRect{ left, 245.0f, left + 270.0f, 485.0f }.Contains(position))
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
			const float left = 300.0f + static_cast<float>(index) * 215.0f;
			if (UiRect{ left, 190.0f, left + 200.0f, 520.0f }.Contains(position))
			{
				return { UiCommand::BuyShopOffer, index };
			}
		}
		if (UiRect{ 340.0f, 640.0f, 660.0f, 690.0f }.Contains(position))
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
	case UiScreenKind::Reward:
		return ResolveRewardClick(position);
	case UiScreenKind::Shop:
		return ResolveShopClick(position);
	case UiScreenKind::Result:
		return ResolveResultClick(position);
	}
	return {};
}
