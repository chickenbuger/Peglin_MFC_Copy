#pragma once

#include "StageDefinition.h"

#include <algorithm>
#include <iterator>

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

enum class KeyboardAction
{
	Pause,
	CombatLog
};

enum class KeyboardBinding
{
	Space,
	KeyP,
	KeyH,
	KeyC
};

enum class MouseAimBinding
{
	LeftButton,
	RightButton
};

inline unsigned int KeyboardBindingVirtualKey(KeyboardBinding binding) noexcept
{
	switch (binding)
	{
	case KeyboardBinding::Space: return 0x20U;
	case KeyboardBinding::KeyP: return static_cast<unsigned int>('P');
	case KeyboardBinding::KeyH: return static_cast<unsigned int>('H');
	case KeyboardBinding::KeyC: return static_cast<unsigned int>('C');
	}
	return 0U;
}

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
	KeyboardBinding pauseBinding = KeyboardBinding::Space;
	KeyboardBinding combatLogBinding = KeyboardBinding::KeyH;
	MouseAimBinding mouseAimBinding = MouseAimBinding::LeftButton;

	bool HasKeyboardBindingConflict() const noexcept
	{
		return pauseBinding == combatLogBinding;
	}

	bool TrySetKeyboardBinding(KeyboardAction action, KeyboardBinding binding) noexcept
	{
		if (KeyboardBindingVirtualKey(binding) == 0U)
		{
			return false;
		}
		if ((action == KeyboardAction::Pause && binding == combatLogBinding)
			|| (action == KeyboardAction::CombatLog && binding == pauseBinding))
		{
			return false;
		}
		if (action == KeyboardAction::Pause)
		{
			pauseBinding = binding;
		}
		else
		{
			combatLogBinding = binding;
		}
		return true;
	}

	bool CycleKeyboardBinding(KeyboardAction action) noexcept
	{
		static constexpr KeyboardBinding bindings[]{
			KeyboardBinding::Space,
			KeyboardBinding::KeyP,
			KeyboardBinding::KeyH,
			KeyboardBinding::KeyC
		};
		const KeyboardBinding current = action == KeyboardAction::Pause
			? pauseBinding
			: combatLogBinding;
		std::size_t currentIndex = 0;
		for (std::size_t index = 0; index < std::size(bindings); ++index)
		{
			if (bindings[index] == current)
			{
				currentIndex = index;
				break;
			}
		}
		bool skippedConflict = false;
		for (std::size_t offset = 1; offset <= std::size(bindings); ++offset)
		{
			const KeyboardBinding candidate = bindings[
				(currentIndex + offset) % std::size(bindings)];
			if (TrySetKeyboardBinding(action, candidate))
			{
				return skippedConflict;
			}
			skippedConflict = true;
		}
		return skippedConflict;
	}

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

	void ToggleMouseAimBinding() noexcept
	{
		mouseAimBinding = mouseAimBinding == MouseAimBinding::LeftButton
			? MouseAimBinding::RightButton
			: MouseAimBinding::LeftButton;
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
