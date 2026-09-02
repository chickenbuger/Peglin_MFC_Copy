#pragma once

#include "StageDefinition.h"

#include <string>
#include <vector>

struct StageDifficultyRating
{
	std::string stageId;
	float score = 0.0f;
	float effectiveEnemyHealth = 0.0f;
};

struct DifficultyCurveAnalysis
{
	bool passed = false;
	std::vector<StageDifficultyRating> ratings;
	std::vector<std::string> issues;
};

DifficultyCurveAnalysis AnalyzeDifficultyCurve(
	const std::vector<StageDefinition>& stages);
