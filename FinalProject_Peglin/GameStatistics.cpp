#include "pch.h"
#include "GameStatistics.h"

#include <algorithm>
#include <tuple>

namespace
{
	bool MatchesDifficulty(
		GameDifficulty difficulty,
		StatisticsDifficultyFilter filter) noexcept
	{
		switch (filter)
		{
		case StatisticsDifficultyFilter::All: return true;
		case StatisticsDifficultyFilter::Easy: return difficulty == GameDifficulty::Easy;
		case StatisticsDifficultyFilter::Normal: return difficulty == GameDifficulty::Normal;
		case StatisticsDifficultyFilter::Hard: return difficulty == GameDifficulty::Hard;
		}
		return true;
	}
}

StatisticsDifficultyFilter NextStatisticsDifficultyFilter(
	StatisticsDifficultyFilter filter) noexcept
{
	switch (filter)
	{
	case StatisticsDifficultyFilter::All: return StatisticsDifficultyFilter::Easy;
	case StatisticsDifficultyFilter::Easy: return StatisticsDifficultyFilter::Normal;
	case StatisticsDifficultyFilter::Normal: return StatisticsDifficultyFilter::Hard;
	case StatisticsDifficultyFilter::Hard: return StatisticsDifficultyFilter::All;
	}
	return StatisticsDifficultyFilter::All;
}

StatisticsSortMode NextStatisticsSortMode(StatisticsSortMode mode) noexcept
{
	switch (mode)
	{
	case StatisticsSortMode::HighScore: return StatisticsSortMode::ClearCount;
	case StatisticsSortMode::ClearCount: return StatisticsSortMode::AverageScore;
	case StatisticsSortMode::AverageScore: return StatisticsSortMode::HighScore;
	}
	return StatisticsSortMode::HighScore;
}

std::vector<PerformanceRecord> BuildStatisticsRows(
	const GameRecordBook& records,
	StatisticsDifficultyFilter filter,
	StatisticsSortMode sortMode)
{
	std::vector<PerformanceRecord> rows;
	for (const PerformanceRecord& record : records.GetAllPerformance())
	{
		if (MatchesDifficulty(record.difficulty, filter)) rows.push_back(record);
	}
	std::sort(rows.begin(), rows.end(), [sortMode](const PerformanceRecord& left, const PerformanceRecord& right)
	{
		if (sortMode == StatisticsSortMode::HighScore && left.highScore != right.highScore)
			return left.highScore > right.highScore;
		if (sortMode == StatisticsSortMode::ClearCount && left.clearCount != right.clearCount)
			return left.clearCount > right.clearCount;
		if (sortMode == StatisticsSortMode::AverageScore && left.AverageScore() != right.AverageScore())
			return left.AverageScore() > right.AverageScore();
		return std::tie(left.stageId, left.difficulty, left.orbId)
			< std::tie(right.stageId, right.difficulty, right.orbId);
	});
	return rows;
}
