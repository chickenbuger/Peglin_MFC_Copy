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

enum class UiLanguage
{
	Korean,
	English
};

struct GameOptions
{
	GameDifficulty difficulty = GameDifficulty::Normal;
	bool soundEnabled = true;
	bool showGameplayInfo = true;
	PegColorMode pegColorMode = PegColorMode::Standard;
	UiLanguage language = UiLanguage::Korean;

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

	void ToggleGameplayInfo() noexcept
	{
		showGameplayInfo = !showGameplayInfo;
	}

	void TogglePegColorMode() noexcept
	{
		pegColorMode = pegColorMode == PegColorMode::Standard
			? PegColorMode::HighContrast
			: PegColorMode::Standard;
	}

	void ToggleLanguage() noexcept
	{
		language = language == UiLanguage::Korean
			? UiLanguage::English
			: UiLanguage::Korean;
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
		for (EnemyDefinition& enemy : stage.enemies)
		{
			enemy.health *= 0.8f;
		}
		stage.rules.playerDamage *= 0.75f;
		stage.rules.enemyStepsBeforeAttack += 2;
		for (EnemyActionDefinition& action : stage.enemyPattern)
		{
			if (action.type == EnemyActionType::Strike)
			{
				action.magnitude *= 0.75f;
			}
		}
		break;
	case GameDifficulty::Normal:
		break;
	case GameDifficulty::Hard:
		stage.rules.enemyHealth *= 1.5f;
		for (EnemyDefinition& enemy : stage.enemies)
		{
			enemy.health *= 1.5f;
		}
		stage.rules.playerDamage *= 1.25f;
		stage.rules.enemyStepsBeforeAttack = (std::max)(
			1,
			stage.rules.enemyStepsBeforeAttack - 2);
		for (EnemyActionDefinition& action : stage.enemyPattern)
		{
			if (action.type == EnemyActionType::Strike)
			{
				action.magnitude *= 1.25f;
			}
		}
		break;
	}

	return stage;
}
