#pragma once

#include "GameRecordStore.h"

#include <vector>

enum class StatisticsDifficultyFilter
{
	All,
	Easy,
	Normal,
	Hard
};

enum class StatisticsSortMode
{
	HighScore,
	ClearCount,
	AverageScore
};

StatisticsDifficultyFilter NextStatisticsDifficultyFilter(
	StatisticsDifficultyFilter filter) noexcept;
StatisticsSortMode NextStatisticsSortMode(StatisticsSortMode mode) noexcept;
std::vector<PerformanceRecord> BuildStatisticsRows(
	const GameRecordBook& records,
	StatisticsDifficultyFilter filter,
	StatisticsSortMode sortMode);
