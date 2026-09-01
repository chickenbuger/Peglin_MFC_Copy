#pragma once

#include "Vector2.h"

#include <algorithm>
#include <cmath>

struct UiViewport
{
	float logicalWidth = 1000.0f;
	float logicalHeight = 700.0f;
	int offsetX = 0;
	int offsetY = 0;
	int pixelWidth = 0;
	int pixelHeight = 0;

	bool IsValid() const noexcept
	{
		return logicalWidth > 0.0f
			&& logicalHeight > 0.0f
			&& pixelWidth > 0
			&& pixelHeight > 0;
	}

	bool ContainsClientPoint(Vector2 point) const noexcept
	{
		return IsValid()
			&& point.x >= static_cast<float>(offsetX)
			&& point.y >= static_cast<float>(offsetY)
			&& point.x <= static_cast<float>(offsetX + pixelWidth)
			&& point.y <= static_cast<float>(offsetY + pixelHeight);
	}

	bool TryClientToLogical(Vector2 point, Vector2& logical, bool clampToViewport = false) const noexcept
	{
		if (!IsValid() || (!clampToViewport && !ContainsClientPoint(point)))
		{
			return false;
		}
		const float localX = std::clamp(
			point.x - static_cast<float>(offsetX),
			0.0f,
			static_cast<float>(pixelWidth));
		const float localY = std::clamp(
			point.y - static_cast<float>(offsetY),
			0.0f,
			static_cast<float>(pixelHeight));
		logical = {
			localX * logicalWidth / static_cast<float>(pixelWidth),
			localY * logicalHeight / static_cast<float>(pixelHeight)
		};
		return true;
	}

	Vector2 LogicalToClient(Vector2 logical) const noexcept
	{
		if (!IsValid()) return {};
		return {
			static_cast<float>(offsetX)
				+ logical.x * static_cast<float>(pixelWidth) / logicalWidth,
			static_cast<float>(offsetY)
				+ logical.y * static_cast<float>(pixelHeight) / logicalHeight
		};
	}
};

inline UiViewport CreateUiViewport(
	int clientWidth,
	int clientHeight,
	float logicalWidth = 1000.0f,
	float logicalHeight = 700.0f) noexcept
{
	UiViewport viewport;
	viewport.logicalWidth = logicalWidth;
	viewport.logicalHeight = logicalHeight;
	if (clientWidth <= 0 || clientHeight <= 0
		|| logicalWidth <= 0.0f || logicalHeight <= 0.0f)
	{
		return viewport;
	}

	const float scale = (std::min)(
		static_cast<float>(clientWidth) / logicalWidth,
		static_cast<float>(clientHeight) / logicalHeight);
	viewport.pixelWidth = (std::max)(1, static_cast<int>(std::floor(logicalWidth * scale)));
	viewport.pixelHeight = (std::max)(1, static_cast<int>(std::floor(logicalHeight * scale)));
	viewport.offsetX = (clientWidth - viewport.pixelWidth) / 2;
	viewport.offsetY = (clientHeight - viewport.pixelHeight) / 2;
	return viewport;
}
