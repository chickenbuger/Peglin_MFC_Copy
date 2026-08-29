#include "pch.h"
#include "PegLayout.h"

#include "GameLayout.h"

#include <algorithm>
#include <random>

namespace
{
	float NextOffset(std::mt19937& generator, float jitter)
	{
		const float normalized = static_cast<float>(generator())
			/ static_cast<float>((std::mt19937::max)());
		return (normalized * 2.0f - 1.0f) * jitter;
	}

	Vector2 ClampToBoard(Vector2 position)
	{
		position.x = std::clamp(
			position.x,
			GameLayout::BoardLeft + GameLayout::PegRadius,
			GameLayout::BoardRight - GameLayout::PegRadius);
		position.y = std::clamp(
			position.y,
			GameLayout::BoardTop + GameLayout::PegRadius,
			GameLayout::BoardBottom - GameLayout::PegRadius);
		return position;
	}
}

PegLayoutDefinition CreateGridPegLayout(
	int columns,
	int rows,
	Vector2 start,
	float spacing)
{
	PegLayoutDefinition layout;
	if (columns <= 0 || rows <= 0)
	{
		return layout;
	}

	layout.positions.reserve(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));
	for (int column = 0; column < columns; ++column)
	{
		for (int row = 0; row < rows; ++row)
		{
			layout.positions.push_back({
				start.x + static_cast<float>(column) * spacing,
				start.y + static_cast<float>(row) * spacing });
		}
	}

	return layout;
}

PegLayoutDefinition CreateSeededPegLayout(
	int columns,
	int rows,
	Vector2 start,
	float spacing,
	float jitter,
	std::uint32_t seed)
{
	PegLayoutDefinition layout = CreateGridPegLayout(columns, rows, start, spacing);
	std::mt19937 generator(seed);
	const float safeJitter = jitter > 0.0f ? jitter : 0.0f;

	for (auto& position : layout.positions)
	{
		position.x += NextOffset(generator, safeJitter);
		position.y += NextOffset(generator, safeJitter);
		position = ClampToBoard(position);
	}

	return layout;
}

PegLayoutDefinition CreateDefaultPegLayout()
{
	return CreateGridPegLayout(
		GameLayout::PegColumns,
		GameLayout::PegRows,
		GameLayout::PegStart,
		GameLayout::PegSpacing);
}
