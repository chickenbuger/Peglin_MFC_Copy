#pragma once

#include "UiNavigation.h"

#include <cstddef>

struct UiFocusRect
{
	float left = 0.0f;
	float top = 0.0f;
	float right = 0.0f;
	float bottom = 0.0f;

	bool IsValid() const noexcept { return right > left && bottom > top; }
};

std::size_t GetGamepadFocusCount(
	UiScreenKind screen,
	std::size_t visibleStageCount) noexcept;
std::size_t MoveGamepadFocus(
	std::size_t currentIndex,
	std::size_t focusCount,
	int direction) noexcept;
UiAction ResolveGamepadFocusedAction(
	UiScreenKind screen,
	std::size_t focusIndex,
	std::size_t visibleStageCount) noexcept;
UiAction ResolveGamepadBackAction(UiScreenKind screen) noexcept;
UiFocusRect GetGamepadFocusRect(
	UiScreenKind screen,
	std::size_t focusIndex,
	std::size_t visibleStageCount) noexcept;
