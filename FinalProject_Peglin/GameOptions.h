#pragma once

#include "StageDefinition.h"

#include <algorithm>

enum class GameDifficulty
{
	Easy,
	Normal,
	Hard
};

enum class PegColorMode
{
	Standard,
	HighContrast
};

struct GameOptions
{
	GameDifficulty difficulty = GameDifficulty::Normal;
	bool soundEnabled = true;
	PegColorMode pegColorMode = PegColorMode::Standard;

	void CycleDifficulty() noexcept
	{
		switch (difficulty)
		{
		case GameDifficulty::Easy: difficulty = GameDifficulty::Normal; break;
		case GameDifficulty::Normal: difficulty = GameDifficulty::Hard; break;
		case GameDifficulty::Hard: difficulty = GameDifficulty::Easy; break;
		}
	}

	void ToggleSound() noexcept
	{
		soundEnabled = !soundEnabled;
	}

	void TogglePegColorMode() noexcept
	{
		pegColorMode = pegColorMode == PegColorMode::Standard
			? PegColorMode::HighContrast
			: PegColorMode::Standard;
	}
};

inline StageDefinition ApplyDifficulty(
	const StageDefinition& baseStage,
	GameDifficulty difficulty)
{
	StageDefinition stage = baseStage;
	switch (difficulty)
	{
	case GameDifficulty::Easy:
		stage.rules.enemyHealth *= 0.8f;
		stage.rules.playerDamage *= 0.75f;
		stage.rules.enemyStepsBeforeAttack += 2;
		break;
	case GameDifficulty::Normal:
		break;
	case GameDifficulty::Hard:
		stage.rules.enemyHealth *= 1.5f;
		stage.rules.playerDamage *= 1.25f;
		stage.rules.enemyStepsBeforeAttack = (std::max)(
			1,
			stage.rules.enemyStepsBeforeAttack - 2);
		break;
	}

	return stage;
}
