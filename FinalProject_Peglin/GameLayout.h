#pragma once

#include "Vector2.h"

struct GameLayout
{
	inline static constexpr float WindowWidth = 1000.0f;
	inline static constexpr float WindowHeight = 800.0f;
	inline static constexpr float SceneWidth = 980.0f;
	inline static constexpr float HeaderHeight = 200.0f;

	inline static constexpr float BoardLeft = 20.0f;
	inline static constexpr float BoardRight = 960.0f;
	inline static constexpr float BoardTop = 200.0f;
	inline static constexpr float BoardBottom = 700.0f;

	inline static constexpr float BallRadius = 10.0f;
	inline static constexpr Vector2 BallInitialPosition{ 490.0f, 250.0f };
	inline static constexpr float BallLeftBoundary = BoardLeft + BallRadius + 5.0f;
	inline static constexpr float BallRightBoundary = BoardRight - BallRadius - 5.0f;
	inline static constexpr float BallTopBoundary = BoardTop + BallRadius + 5.0f;
	inline static constexpr float BallExitY = WindowHeight;

	inline static constexpr int PegColumns = 12;
	inline static constexpr int PegRows = 4;
	inline static constexpr Vector2 PegStart{ 50.0f, 400.0f };
	inline static constexpr float PegSpacing = 80.0f;
	inline static constexpr float PegRadius = 10.0f;

	inline static constexpr Vector2 PlayerPosition{ 125.0f, 112.0f };
	inline static constexpr Vector2 PlayerSize{ 104.0f, 108.0f };
	inline static constexpr Vector2 PlayerHealthText{ 100.0f, 70.0f };

	inline static constexpr Vector2 EnemyInitialPosition{ 710.0f, 114.0f };
	inline static constexpr Vector2 EnemySize{ 132.0f, 106.0f };
	inline static constexpr float EnemyGroupStartX = 640.0f;
	inline static constexpr float EnemyGroupSpacing = 105.0f;
	inline static constexpr float EnemyGroupY = 136.0f;
	inline static constexpr Vector2 EnemyGroupSize{ 96.0f, 84.0f };
	inline static constexpr float EnemyStep = 64.0f;
	inline static constexpr int EnemyStepsBeforeAttack = 8;
	inline static constexpr float EnemyHealthTextOffsetX = -30.0f;
	inline static constexpr float EnemyHealthTextY = 90.0f;
	inline static constexpr Vector2 StateText{ 330.0f, 70.0f };
	inline static constexpr Vector2 FeedbackText{ 330.0f, 90.0f };
	inline static constexpr Vector2 OptionsText{ 330.0f, 110.0f };
	inline static constexpr Vector2 TurnEffectPosition{ 490.0f, 150.0f };
	inline static constexpr Vector2 AimStrengthPosition{ 30.0f, 290.0f };
	inline static constexpr float AimStrengthWidth = 160.0f;
	inline static constexpr float AimStrengthHeight = 14.0f;
};
