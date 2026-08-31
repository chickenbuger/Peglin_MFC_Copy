#include "pch.h"
#include "RewardPresentation.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <vector>

namespace
{
	bool IsNeutral(float multiplier) noexcept
	{
		return std::abs(multiplier - 1.0f) <= 0.0001f;
	}

	std::string Fixed(float value, int precision)
	{
		std::ostringstream output;
		output << std::fixed << std::setprecision(precision) << value;
		return output.str();
	}

	std::string PercentChange(float multiplier)
	{
		const float percent = (multiplier - 1.0f) * 100.0f;
		if (std::abs(percent) <= 0.0001f)
		{
			return "변동 없음";
		}
		std::ostringstream output;
		output << (percent > 0.0f ? '+' : '-')
			<< std::fixed << std::setprecision(
				std::abs(percent - std::round(percent)) <= 0.001f ? 0 : 2)
			<< std::abs(percent) << '%';
		return output.str();
	}

	std::string Modifier(std::string_view label, float multiplier, int precision = 2)
	{
		return std::string(label) + " ×" + Fixed(multiplier, precision)
			+ " (" + PercentChange(multiplier) + ')';
	}

	std::string Join(const std::vector<std::string>& lines)
	{
		std::string result;
		for (const std::string& line : lines)
		{
			if (!result.empty())
			{
				result += '\n';
			}
			result += line;
		}
		return result;
	}
}

std::string DescribeOrbEffect(const OrbDefinition& orb)
{
	std::vector<std::string> lines;
	lines.push_back(Modifier("페그 피해", orb.pegDamageMultiplier));
	lines.push_back(
		Modifier("획득 점수", orb.scoreMultiplier)
		+ " · "
		+ (orb.attackDelivery == AttackDelivery::Melee ? "근접" : "원거리")
		+ " · "
		+ (orb.attackTarget == AttackTarget::All ? "모든 적" : "단일 대상"));
	lines.emplace_back("획득 시 오브 덱에 1개 추가");
	return Join(lines);
}

std::string DescribeRelicEffect(const RelicDefinition& relic)
{
	std::vector<std::string> lines;
	if (!IsNeutral(relic.pegDamageMultiplier))
	{
		lines.push_back(Modifier("페그 피해", relic.pegDamageMultiplier));
	}
	if (!IsNeutral(relic.scoreMultiplier))
	{
		lines.push_back(Modifier("획득 점수", relic.scoreMultiplier));
	}
	if (!IsNeutral(relic.incomingDamageMultiplier))
	{
		lines.push_back(Modifier("받는 피해", relic.incomingDamageMultiplier));
	}

	if (relic.duplicatePolicy == RelicDuplicatePolicy::Unique)
	{
		lines.emplace_back("고유 유물 · 1개만 보유 가능");
	}
	else
	{
		lines.push_back(
			"중첩 유물 · 최대 " + std::to_string(relic.maxStacks) + "개 · 배율 곱연산");
		if (relic.maxStacks > 1 && !IsNeutral(relic.incomingDamageMultiplier))
		{
			const float maximumMultiplier = std::pow(
				relic.incomingDamageMultiplier,
				static_cast<float>(relic.maxStacks));
			lines.push_back(
				std::to_string(relic.maxStacks) + "중첩 시 "
				+ Modifier("받는 피해", maximumMultiplier, 4));
		}
	}
	return Join(lines);
}

std::string DescribeRewardEffect(const RunReward& reward)
{
	switch (reward.kind)
	{
	case RunRewardKind::Orb:
	{
		const OrbDefinition* orb = FindOrbDefinition(reward.id);
		return orb == nullptr ? "등록되지 않은 오브" : DescribeOrbEffect(*orb);
	}
	case RunRewardKind::Relic:
	{
		const RelicDefinition* relic = FindRelicDefinition(reward.id);
		return relic == nullptr ? "등록되지 않은 유물" : DescribeRelicEffect(*relic);
	}
	case RunRewardKind::Heal:
		return "체력 " + Fixed(reward.magnitude, 0)
			+ " 회복 · 최대 체력을 초과하지 않음";
	}
	return {};
}
