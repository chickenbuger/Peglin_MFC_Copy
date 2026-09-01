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

enum class GamepadFireBinding
{
	SouthButton,
	RightTrigger
};

struct GameOptions
{
	GameDifficulty difficulty = GameDifficulty::Normal;
	bool soundEnabled = true;
	int effectsVolume = 70;
	int musicVolume = 45;
	bool showGameplayInfo = true;
	PegColorMode pegColorMode = PegColorMode::Standard;
	UiLanguage language = UiLanguage::Korean;
	int gamepadDeadzonePercent = 25;
	int gamepadSensitivityPercent = 100;
	GamepadFireBinding gamepadFireBinding = GamepadFireBinding::SouthButton;

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

	void CycleEffectsVolume() noexcept
	{
		effectsVolume = effectsVolume >= 100 ? 0 : effectsVolume + 25;
	}

	void CycleMusicVolume() noexcept
	{
		musicVolume = musicVolume >= 100 ? 0 : musicVolume + 25;
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

	void CycleGamepadDeadzone() noexcept
	{
		if (gamepadDeadzonePercent < 25) gamepadDeadzonePercent = 25;
		else if (gamepadDeadzonePercent < 35) gamepadDeadzonePercent = 35;
		else gamepadDeadzonePercent = 15;
	}

	void CycleGamepadSensitivity() noexcept
	{
		if (gamepadSensitivityPercent < 100) gamepadSensitivityPercent = 100;
		else if (gamepadSensitivityPercent < 125) gamepadSensitivityPercent = 125;
		else if (gamepadSensitivityPercent < 150) gamepadSensitivityPercent = 150;
		else gamepadSensitivityPercent = 75;
	}

	void ToggleGamepadFireBinding() noexcept
	{
		gamepadFireBinding = gamepadFireBinding == GamepadFireBinding::SouthButton
			? GamepadFireBinding::RightTrigger
			: GamepadFireBinding::SouthButton;
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
