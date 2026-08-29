#include "pch.h"
#include "PegLayout.h"

#include "GameLayout.h"

#include <algorithm>
#include <random>

namespace
{
	constexpr PegTypeDefinition NORMAL_PEG{
		PegType::Normal, { 255, 0, 0 }, 1.0f, 1, 0.0f, false };
	constexpr PegTypeDefinition CRITICAL_PEG{
		PegType::Critical, { 255, 215, 0 }, 2.0f, 2, 0.0f, false };
	constexpr PegTypeDefinition BOMB_PEG{
		PegType::Bomb, { 48, 48, 48 }, 1.0f, 1, 100.0f, false };
	constexpr PegTypeDefinition REFRESH_PEG{
		PegType::Refresh, { 0, 200, 90 }, 1.0f, 1, 0.0f, true };

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
			GameLayout::PegFieldLeft + GameLayout::PegRadius,
			GameLayout::PegFieldRight - GameLayout::PegRadius);
		position.y = std::clamp(
			position.y,
			GameLayout::PegFieldTop + GameLayout::PegRadius,
			GameLayout::PegFieldBottom - GameLayout::PegRadius);
		return position;
	}
}

const PegTypeDefinition& GetPegTypeDefinition(PegType type) noexcept
{
	switch (type)
	{
	case PegType::Critical:
		return CRITICAL_PEG;
	case PegType::Bomb:
		return BOMB_PEG;
	case PegType::Refresh:
		return REFRESH_PEG;
	case PegType::Normal:
	default:
		return NORMAL_PEG;
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

	layout.pegs.reserve(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));
	for (int column = 0; column < columns; ++column)
	{
		for (int row = 0; row < rows; ++row)
		{
			layout.pegs.push_back({ {
				start.x + static_cast<float>(column) * spacing,
				start.y + static_cast<float>(row) * spacing }, PegType::Normal });
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

	for (auto& peg : layout.pegs)
	{
		peg.position.x += NextOffset(generator, safeJitter);
		peg.position.y += NextOffset(generator, safeJitter);
		peg.position = ClampToBoard(peg.position);
	}

	return layout;
}

PegLayoutDefinition CreateDefaultPegLayout()
{
	PegLayoutDefinition layout = CreateGridPegLayout(
		GameLayout::PegColumns,
		GameLayout::PegRows,
		GameLayout::PegStart,
		GameLayout::PegSpacing);

	if (layout.pegs.size() == 48)
	{
		layout.pegs[5].type = PegType::Critical;
		layout.pegs[18].type = PegType::Bomb;
		layout.pegs[29].type = PegType::Refresh;
		layout.pegs[41].type = PegType::Critical;
	}

	return layout;
}
