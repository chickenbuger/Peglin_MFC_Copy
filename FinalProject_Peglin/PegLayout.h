#pragma once

#include "Vector2.h"

#include <cstdint>
#include <vector>

struct PegLayoutDefinition
{
	std::vector<Vector2> positions;
};

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
