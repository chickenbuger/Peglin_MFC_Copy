#pragma once

#include "Vector2.h"

#include <cstdint>
#include <vector>

enum class PegType
{
	Normal,
	Critical,
	Bomb,
	Refresh
};

struct PegVisualStyle
{
	std::uint8_t red = 255;
	std::uint8_t green = 0;
	std::uint8_t blue = 0;
};

struct PegTypeDefinition
{
	PegType type = PegType::Normal;
	PegVisualStyle visual;
	float damage = 1.0f;
	int scoreMultiplier = 1;
	float blastRadius = 0.0f;
	bool refreshesRemovedPegs = false;
};

struct PegDefinition
{
	Vector2 position;
	PegType type = PegType::Normal;
};

struct PegLayoutDefinition
{
	std::vector<PegDefinition> pegs;
};

const PegTypeDefinition& GetPegTypeDefinition(PegType type) noexcept;

PegLayoutDefinition CreateGridPegLayout(
	int columns,
	int rows,
	Vector2 start,
	float spacing);

PegLayoutDefinition CreateSeededPegLayout(
	int columns,
	int rows,
	Vector2 start,
	float spacing,
	float jitter,
	std::uint32_t seed);

PegLayoutDefinition CreateDefaultPegLayout();
