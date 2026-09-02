#include "pch.h"
#include "DifficultyCurve.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float MAX_ADJACENT_DROP = 2.0f;
	constexpr float MAX_ADJACENT_RISE = 4.0f;
	constexpr float MAX_BRANCH_SPREAD = 2.5f;
	constexpr float LAYER_DROP_TOLERANCE = 0.5f;

	std::size_t CountMovingPegs(const StageDefinition& stage) noexcept
	{
		return static_cast<std::size_t>(std::count_if(
			stage.pegLayout.pegs.begin(),
			stage.pegLayout.pegs.end(),
			[](const PegDefinition& peg) { return peg.motion.IsMoving(); }));
	}

	float EffectiveEnemyHealth(const StageDefinition& stage) noexcept
	{
		float total = 0.0f;
		for (const EnemyDefinition& enemy : stage.enemies)
		{
			total += enemy.health / (std::max)(enemy.damageTakenMultiplier, 0.1f);
		}
		return total;
	}

	float RateStage(const StageDefinition& stage, float effectiveEnemyHealth) noexcept
	{
		const float attackTurns = effectiveEnemyHealth
			/ (std::max)(stage.rules.playerDamage, 0.1f);
		float averageRange = 0.0f;
		for (const EnemyDefinition& enemy : stage.enemies)
		{
			averageRange += static_cast<float>(enemy.attackRangeCells);
		}
		if (!stage.enemies.empty())
		{
			averageRange /= static_cast<float>(stage.enemies.size());
		}
		const float approachPressure = static_cast<float>(
			(std::max)(0, 8 - stage.rules.enemyStepsBeforeAttack));
		return attackTurns * 4.0f
			+ static_cast<float>(stage.enemies.size()) * 0.65f
			+ averageRange * 0.35f
			+ static_cast<float>(CountMovingPegs(stage)) * 0.18f
			+ approachPressure * 0.45f
			+ (stage.isBoss ? 6.0f : 0.0f);
	}

	void AddIssue(DifficultyCurveAnalysis& analysis, std::string issue)
	{
		analysis.issues.push_back(std::move(issue));
	}
}

DifficultyCurveAnalysis AnalyzeDifficultyCurve(
	const std::vector<StageDefinition>& stages)
{
	DifficultyCurveAnalysis analysis;
	analysis.ratings.reserve(stages.size());
	for (const StageDefinition& stage : stages)
	{
		const float effectiveHealth = EffectiveEnemyHealth(stage);
		analysis.ratings.push_back({
			stage.id,
			RateStage(stage, effectiveHealth),
			effectiveHealth
		});
	}

	if (stages.size() < 4)
	{
		AddIssue(analysis, "At least three regular stages and one boss are required.");
	}

	std::size_t bossCount = 0;
	float maximumRegularScore = 0.0f;
	for (std::size_t index = 0; index < stages.size(); ++index)
	{
		if (stages[index].isBoss)
		{
			++bossCount;
			if (index + 1 != stages.size())
			{
				AddIssue(analysis, "The boss must be the final catalog stage.");
			}
		}
		else
		{
			maximumRegularScore = (std::max)(maximumRegularScore, analysis.ratings[index].score);
		}
	}
	if (bossCount != 1)
	{
		AddIssue(analysis, "Exactly one boss stage is required.");
	}

	for (std::size_t index = 1; index < stages.size(); ++index)
	{
		if (stages[index - 1].isBoss || stages[index].isBoss) continue;
		const float delta = analysis.ratings[index].score - analysis.ratings[index - 1].score;
		if (delta < -MAX_ADJACENT_DROP || delta > MAX_ADJACENT_RISE)
		{
			AddIssue(analysis, "Adjacent difficulty jump exceeds the regular-stage budget at "
				+ stages[index].id + ".");
		}
	}

	float previousLayerAverage = -1.0f;
	for (std::size_t index = 0; index < stages.size() && !stages[index].isBoss;)
	{
		const std::size_t layerWidth = index == 0 ? 1U : (std::min)(std::size_t{ 2 }, stages.size() - index);
		float layerTotal = 0.0f;
		std::size_t regularCount = 0;
		for (std::size_t offset = 0; offset < layerWidth && !stages[index + offset].isBoss; ++offset)
		{
			layerTotal += analysis.ratings[index + offset].score;
			++regularCount;
		}
		if (regularCount == 0) break;
		const float layerAverage = layerTotal / static_cast<float>(regularCount);
		if (previousLayerAverage >= 0.0f
			&& layerAverage + LAYER_DROP_TOLERANCE < previousLayerAverage)
		{
			AddIssue(analysis, "A route layer is easier than the preceding layer.");
		}
		if (regularCount == 2
			&& std::fabs(analysis.ratings[index].score - analysis.ratings[index + 1].score)
				> MAX_BRANCH_SPREAD)
		{
			AddIssue(analysis, "Parallel route choices have an excessive difficulty spread.");
		}
		previousLayerAverage = layerAverage;
		index += regularCount;
	}

	if (!stages.empty() && stages.back().isBoss && maximumRegularScore > 0.0f)
	{
		const float bossScore = analysis.ratings.back().score;
		if (bossScore < maximumRegularScore * 1.35f
			|| bossScore > maximumRegularScore * 3.0f)
		{
			AddIssue(analysis, "Boss difficulty is outside the 1.35x to 3.00x target band.");
		}
	}

	analysis.passed = analysis.issues.empty();
	return analysis;
}
