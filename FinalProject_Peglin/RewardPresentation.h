#pragma once

#include "Progression.h"
#include "RunProgression.h"

#include <string>

std::string DescribeOrbEffect(const OrbDefinition& orb);
std::string DescribeRelicEffect(const RelicDefinition& relic);
std::string DescribeRewardEffect(const RunReward& reward);
