#include <afxwin.h>

#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

#include "GameWorld.h"
#include "GameLayout.h"
#include "PegLayout.h"
#include "Physics.h"

namespace
{
	int failures = 0;

	void Check(bool condition, std::string_view name)
	{
		if (condition)
		{
			std::cout << "[PASS] " << name << '\n';
		}
		else
		{
			std::cerr << "[FAIL] " << name << '\n';
			++failures;
		}
	}

	bool Near(float actual, float expected, float tolerance = 0.0001f)
	{
		return std::fabs(actual - expected) <= tolerance;
	}

	void Launch(GameWorld& world)
	{
		Check(world.BeginAim({ 100.0f, 100.0f }), "begin aim");
		Check(world.ReleaseShot({ 90.0f, 100.0f }), "launch non-zero shot");
		Check(world.GetState() == GameState::BallInFlight, "shot enters BallInFlight");
	}

	void TestZeroLengthShot()
	{
		GameWorld world;
		Check(world.BeginAim({ 100.0f, 100.0f }), "zero shot begin aim");
		Check(!world.ReleaseShot({ 100.0f, 100.0f }), "zero shot is cancelled");
		Check(world.GetState() == GameState::Aiming, "zero shot remains Aiming");
		Check(!world.GetBall().GetActive(), "zero shot leaves ball inactive");
		Check(Near(world.GetBall().GetVelocity().LengthSquared(), 0.0f), "zero shot velocity is zero");
	}

	void TestWallReflection()
	{
		Parent_ball ball;
		ball.SetPosition({ 34.0f, 300.0f });
		ball.SetVelocity({ -2.0f, 1.0f });
		ball.collision();
		Check(Near(ball.GetPosition().x, GameLayout::BallLeftBoundary), "left wall corrects overlap");
		Check(Near(ball.GetVelocity().x, 2.0f), "left wall reflects approaching velocity");
		Check(Near(ball.GetVelocity().y, 1.0f), "left wall preserves tangent velocity");

		ball.SetPosition({ 500.0f, 214.0f });
		ball.SetVelocity({ 1.0f, -3.0f });
		ball.collision();
		Check(Near(ball.GetPosition().y, GameLayout::BallTopBoundary), "ceiling corrects overlap");
		Check(Near(ball.GetVelocity().y, 3.0f), "ceiling reflects approaching velocity");
	}

	void TestPegReflectionAndSingleDamage()
	{
		GameWorld world;
		world.SetPegRestitution(0.5f);
		Launch(world);

		auto& targets = world.GetTargets()._targetBallList;
		Vector2 pegPosition;
		auto scan = targets.GetHeadPosition();
		while (scan != nullptr)
		{
			const auto& candidate = targets.GetNext(scan);
			if (candidate.position.x > 200.0f && candidate.position.x < 800.0f)
			{
				pegPosition = candidate.position;
				break;
			}
		}
		world.GetBall().SetPosition(pegPosition + Vector2{ -15.0f, 0.0f });
		world.GetBall().SetVelocity({ 2.0f, 3.0f });

		world.Update(0.0f);
		Check(targets.GetCount() == 47, "peg collision removes exactly one peg");
		Check(world.GetFeedback().type == GameFeedbackType::PegHit, "peg collision emits PegHit feedback");
		Check(world.GetFeedback().currentShotPegHits == 1, "peg feedback counts current shot hits");
		Check(world.GetScore().currentCombo == 1, "first peg starts combo at one");
		Check(world.GetScore().currentShot == 100, "first peg awards base combo score");
		Check(Near(world.GetBall().GetVelocity().x, -1.0f), "peg normal speed uses restitution");
		Check(Near(world.GetBall().GetVelocity().y, 3.0f), "peg tangent speed is preserved");
		const float separation = (world.GetBall().GetPosition() - pegPosition).Length();
		Check(separation > 20.0f, "peg overlap is corrected outside collision radius");

		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.GetBall().SetVelocity({ 0.0f, 1.0f });
		world.Update(0.0f);
		const GameUpdateResult result = world.Update(0.0f);
		Check(result == GameUpdateResult::None, "ordinary turn has no terminal result");
		Check(Near(world.GetEnemy().GetHp(), 19.0f), "one removed peg deals exactly one damage");
		Check(world.GetFeedback().type == GameFeedbackType::TurnResolved, "ordinary turn emits TurnResolved feedback");
		Check(Near(world.GetFeedback().lastEnemyDamage, 1.0f), "turn feedback reports enemy damage");
		Check(world.GetScore().lastTurn == 100, "turn stores the completed shot score");
		Check(world.GetScore().total == 100, "turn adds shot score to total");
		Check(world.GetScore().currentCombo == 0, "turn resets current combo");
		Check(world.GetScore().currentShot == 0, "turn resets current shot score");
		Check(world.GetState() == GameState::Aiming, "ordinary turn returns to Aiming");
	}

	void TestScoreComboProgression()
	{
		GameWorld world;
		Launch(world);

		std::vector<Vector2> pegPositions;
		auto scan = world.GetTargets()._targetBallList.GetHeadPosition();
		while (scan != nullptr && pegPositions.size() < 2)
		{
			const auto& candidate = world.GetTargets()._targetBallList.GetNext(scan);
			if (candidate.position.x > 200.0f && candidate.position.x < 800.0f)
			{
				pegPositions.push_back(candidate.position);
			}
		}

		Check(pegPositions.size() == 2, "score test finds two interior pegs");
		for (const Vector2 pegPosition : pegPositions)
		{
			world.GetBall().SetPosition(pegPosition + Vector2{ -15.0f, 0.0f });
			world.GetBall().SetVelocity({ 2.0f, 0.0f });
			world.Update(0.0f);
		}

		Check(world.GetScore().currentCombo == 2, "two hits build a two-step combo");
		Check(world.GetScore().bestCombo == 2, "best combo tracks the shot peak");
		Check(world.GetScore().currentShot == 300, "combo score awards 100 plus 200");
		Check(world.GetScore().total == 0, "shot score stays pending until turn resolution");

		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.GetBall().SetVelocity({ 0.0f, 1.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		Check(world.GetScore().lastTurn == 300, "turn records combo-weighted shot score");
		Check(world.GetScore().total == 300, "turn commits combo-weighted score");
		Check(world.GetScore().currentCombo == 0, "resolved combo returns to zero");
		Check(world.GetScore().currentShot == 0, "resolved shot score returns to zero");

		world.ResetGame();
		Check(world.GetScore().total == 0, "new game resets total score");
		Check(world.GetScore().bestCombo == 0, "new game resets best combo");
	}

	void TestScreenExitTurn()
	{
		GameWorld world;
		Launch(world);
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.GetBall().SetVelocity({ 0.0f, 1.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		Check(world.GetEnemy().GetCount() == 1, "screen exit advances enemy turn count");
		Check(Near(world.GetEnemy().GetX(), GameLayout::EnemyInitialPosition.x - GameLayout::EnemyStep), "screen exit advances enemy position");
		Check(Near(world.GetEnemy().GetHp(), 20.0f), "turn without pegs deals no enemy damage");
	}

	void TestVictoryTransition()
	{
		GameWorld world;
		world.GetEnemy().SetHp(1.0f);
		Launch(world);

		auto& targets = world.GetTargets()._targetBallList;
		Vector2 pegPosition;
		auto scan = targets.GetHeadPosition();
		while (scan != nullptr)
		{
			const auto& candidate = targets.GetNext(scan);
			if (candidate.position.x > 200.0f && candidate.position.x < 800.0f)
			{
				pegPosition = candidate.position;
				break;
			}
		}
		world.GetBall().SetPosition(pegPosition + Vector2{ -15.0f, 0.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		const GameUpdateResult result = world.Update(0.0f);
		Check(result == GameUpdateResult::Victory, "lethal peg damage reports Victory");
		Check(world.GetState() == GameState::Victory, "lethal peg damage enters Victory state");
		Check(world.Update(0.0f) == GameUpdateResult::None, "Victory result is reported only once");
		Check(world.Update(0.0f) == GameUpdateResult::None, "Victory state does not repeat its result");
	}

	void TestDefeatTransition()
	{
		GameWorld world;
		world.GetPlayer().SetHp(20.0f);
		world.GetEnemy().SetCount(8);
		Launch(world);
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		const GameUpdateResult result = world.Update(0.0f);
		Check(result == GameUpdateResult::Defeat, "player attack threshold reports Defeat");
		Check(world.GetState() == GameState::Defeat, "player attack threshold enters Defeat state");
		Check(Near(world.GetPlayer().GetHp(), 0.0f), "defeat applies player damage once");
		Check(world.GetFeedback().type == GameFeedbackType::Defeat, "defeat emits Defeat feedback");
		Check(Near(world.GetFeedback().lastPlayerDamage, 20.0f), "defeat feedback reports player damage");
		Check(world.Update(0.0f) == GameUpdateResult::None, "Defeat result is reported only once");
		Check(world.Update(0.0f) == GameUpdateResult::None, "Defeat state does not repeat its result");
	}

	void TestStateAndRestitutionRules()
	{
		Check(CanTransitionTo(GameState::Aiming, GameState::BallInFlight), "valid state transition accepted");
		Check(!CanTransitionTo(GameState::Aiming, GameState::Victory), "invalid state transition rejected");
		Check(Near(ClampRestitution(-1.0f), 0.0f), "negative restitution clamps to zero");
		Check(Near(ClampRestitution(2.0f), 1.0f), "restitution above one clamps to one");
		const Vector2 unchanged = ReflectVelocity({ 2.0f, 3.0f }, { 1.0f, 0.0f }, 0.5f);
		Check(Near(unchanged.x, 2.0f) && Near(unchanged.y, 3.0f), "separating velocity is not reflected");
	}

	void TestLayoutConfiguration()
	{
		GameWorld world;
		Check(Near(world.GetBall().GetPosition().x, GameLayout::BallInitialPosition.x), "layout controls initial ball x");
		Check(Near(world.GetBall().GetPosition().y, GameLayout::BallInitialPosition.y), "layout controls initial ball y");
		Check(Near(world.GetEnemy().GetX(), GameLayout::EnemyInitialPosition.x), "layout controls initial enemy x");
		Check(world.GetTargets()._targetBallList.GetCount() == GameLayout::PegColumns * GameLayout::PegRows, "layout controls peg count");
		const auto head = world.GetTargets()._targetBallList.GetHeadPosition();
		const Vector2 firstPeg = world.GetTargets()._targetBallList.GetAt(head).position;
		Check(Near(firstPeg.x, GameLayout::PegStart.x) && Near(firstPeg.y, GameLayout::PegStart.y), "layout controls first peg position");
	}

	void TestSeededPegLayout()
	{
		const PegLayoutDefinition first = CreateSeededPegLayout(
			GameLayout::PegColumns,
			GameLayout::PegRows,
			GameLayout::PegStart,
			GameLayout::PegSpacing,
			20.0f,
			12345u);
		const PegLayoutDefinition repeated = CreateSeededPegLayout(
			GameLayout::PegColumns,
			GameLayout::PegRows,
			GameLayout::PegStart,
			GameLayout::PegSpacing,
			20.0f,
			12345u);
		const PegLayoutDefinition different = CreateSeededPegLayout(
			GameLayout::PegColumns,
			GameLayout::PegRows,
			GameLayout::PegStart,
			GameLayout::PegSpacing,
			20.0f,
			54321u);

		Check(first.pegs.size() == 48, "seeded layout preserves requested peg count");
		bool sameSeedMatches = first.pegs.size() == repeated.pegs.size();
		bool differentSeedDiffers = false;
		bool allInsideBoard = true;
		for (std::size_t index = 0; index < first.pegs.size(); ++index)
		{
			const Vector2 position = first.pegs[index].position;
			sameSeedMatches = sameSeedMatches
				&& Near(position.x, repeated.pegs[index].position.x)
				&& Near(position.y, repeated.pegs[index].position.y);
			differentSeedDiffers = differentSeedDiffers
				|| !Near(position.x, different.pegs[index].position.x)
				|| !Near(position.y, different.pegs[index].position.y);
			allInsideBoard = allInsideBoard
				&& position.x >= GameLayout::BoardLeft + GameLayout::PegRadius
				&& position.x <= GameLayout::BoardRight - GameLayout::PegRadius
				&& position.y >= GameLayout::BoardTop + GameLayout::PegRadius
				&& position.y <= GameLayout::BoardBottom - GameLayout::PegRadius;
		}

		Check(sameSeedMatches, "same seed reproduces identical peg positions");
		Check(differentSeedDiffers, "different seed changes peg positions");
		Check(allInsideBoard, "seeded jitter keeps every peg inside board");
	}

	void TestDataDrivenPegLayout()
	{
		PegLayoutDefinition custom;
		custom.pegs = {
			{ { 300.0f, 400.0f }, PegType::Normal },
			{ { 500.0f, 500.0f }, PegType::Critical },
			{ { 700.0f, 600.0f }, PegType::Bomb }
		};
		GameWorld world(custom);

		Check(world.GetPegLayout().pegs.size() == 3, "game world retains custom layout data");
		Check(world.GetTargets()._targetBallList.GetCount() == 3, "custom layout controls target count");
		const auto head = world.GetTargets()._targetBallList.GetHeadPosition();
		const Vector2 firstPeg = world.GetTargets()._targetBallList.GetAt(head).position;
		Check(Near(firstPeg.x, 300.0f) && Near(firstPeg.y, 400.0f), "custom layout controls peg positions");
		Check(world.GetTargets()._targetBallList.GetAt(head).type == PegType::Normal, "custom layout controls peg type");
		world.ResetGame();
		Check(world.GetTargets()._targetBallList.GetCount() == 3, "reset rebuilds the configured custom layout");
	}

	void TestPegTypeDefinitions()
	{
		const PegTypeDefinition& normal = GetPegTypeDefinition(PegType::Normal);
		const PegTypeDefinition& critical = GetPegTypeDefinition(PegType::Critical);
		const PegTypeDefinition& bomb = GetPegTypeDefinition(PegType::Bomb);
		const PegTypeDefinition& refresh = GetPegTypeDefinition(PegType::Refresh);

		Check(Near(normal.damage, 1.0f) && normal.scoreMultiplier == 1, "normal peg keeps baseline rewards");
		Check(Near(critical.damage, 2.0f) && critical.scoreMultiplier == 2, "critical peg doubles damage and score");
		Check(Near(bomb.blastRadius, 100.0f), "bomb peg defines a data-driven blast radius");
		Check(refresh.refreshesRemovedPegs, "refresh peg defines board restoration effect");
		Check(normal.visual.red == 255 && normal.visual.green == 0, "normal peg visual style remains red");
		Check(critical.visual.red == 255 && critical.visual.green == 215, "critical peg visual style is distinct");

		const PegLayoutDefinition layout = CreateDefaultPegLayout();
		int normalCount = 0;
		int criticalCount = 0;
		int bombCount = 0;
		int refreshCount = 0;
		for (const PegDefinition& peg : layout.pegs)
		{
			switch (peg.type)
			{
			case PegType::Normal: ++normalCount; break;
			case PegType::Critical: ++criticalCount; break;
			case PegType::Bomb: ++bombCount; break;
			case PegType::Refresh: ++refreshCount; break;
			}
		}
		Check(normalCount == 44, "default board contains 44 normal pegs");
		Check(criticalCount == 2, "default board contains two critical pegs");
		Check(bombCount == 1, "default board contains one bomb peg");
		Check(refreshCount == 1, "default board contains one refresh peg");
	}

	void TestCriticalPegEffect()
	{
		PegLayoutDefinition layout;
		layout.pegs = { { { 500.0f, 500.0f }, PegType::Critical } };
		GameWorld world(layout);
		Launch(world);
		world.GetBall().SetPosition({ 485.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);

		Check(world.GetTargets()._targetBallList.IsEmpty(), "critical collision removes its peg");
		Check(world.GetScore().currentShot == 200, "critical peg doubles first combo score");
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		Check(Near(world.GetEnemy().GetHp(), 18.0f), "critical peg deals two damage");
		Check(world.GetScore().total == 200, "critical score commits at turn end");
	}

	void TestBombPegEffect()
	{
		PegLayoutDefinition layout;
		layout.pegs = {
			{ { 500.0f, 500.0f }, PegType::Bomb },
			{ { 580.0f, 500.0f }, PegType::Normal },
			{ { 720.0f, 500.0f }, PegType::Normal }
		};
		GameWorld world(layout);
		Launch(world);
		world.GetBall().SetPosition({ 485.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);

		Check(world.GetTargets()._targetBallList.GetCount() == 1, "bomb removes only pegs inside blast radius");
		Check(world.GetFeedback().currentShotPegHits == 2, "bomb reward counts itself and nearby peg");
		Check(world.GetScore().currentCombo == 2, "bomb effect advances combo for both pegs");
		Check(world.GetScore().currentShot == 300, "bomb effect awards combo score for both pegs");
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		Check(Near(world.GetEnemy().GetHp(), 18.0f), "bomb and neighbor deal two total damage");
	}

	void TestRefreshPegEffect()
	{
		PegLayoutDefinition layout;
		layout.pegs = {
			{ { 300.0f, 500.0f }, PegType::Normal },
			{ { 500.0f, 500.0f }, PegType::Refresh },
			{ { 700.0f, 500.0f }, PegType::Normal }
		};
		GameWorld world(layout);
		Launch(world);

		world.GetBall().SetPosition({ 285.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		Check(world.GetTargets()._targetBallList.GetCount() == 2, "normal collision removes one peg before refresh");

		world.GetBall().SetPosition({ 485.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		Check(world.GetTargets()._targetBallList.GetCount() == 2, "refresh restores removed pegs but excludes itself");
		Check(world.GetFeedback().currentShotPegHits == 2, "refresh collision counts as one additional hit");
		Check(world.GetScore().currentShot == 300, "refresh hit preserves normal combo scoring");

		bool restoredNormalFound = false;
		auto position = world.GetTargets()._targetBallList.GetHeadPosition();
		while (position != nullptr)
		{
			const TargetBall& target = world.GetTargets()._targetBallList.GetNext(position);
			if (Near(target.position.x, 300.0f) && target.type == PegType::Normal)
			{
				restoredNormalFound = true;
			}
		}
		Check(restoredNormalFound, "refresh restores the removed peg with its original type");
	}

	void TestStageCatalogAndValidation()
	{
		const StageLoadResult defaultResult = LoadStageDefinition("stage-1");
		Check(defaultResult.IsSuccess(), "stage catalog loads stage-1");
		Check(defaultResult.stage->id == "stage-1", "loaded default stage keeps its id");
		Check(defaultResult.stage->pegLayout.pegs.size() == 48, "loaded default stage provides its board");
		Check(Near(defaultResult.stage->rules.enemyHealth, 20.0f), "loaded default stage provides enemy health");

		const StageLoadResult challengeResult = LoadStageDefinition("stage-2");
		Check(challengeResult.IsSuccess(), "stage catalog loads stage-2");
		Check(challengeResult.stage->pegLayout.pegs.size() == 40, "challenge stage provides a distinct board");
		Check(Near(challengeResult.stage->rules.enemyHealth, 30.0f), "challenge stage provides stronger enemy");
		Check(challengeResult.stage->rules.enemyStepsBeforeAttack == 6, "challenge stage provides faster attack timing");

		const StageLoadResult missingResult = LoadStageDefinition("missing-stage");
		Check(!missingResult.IsSuccess(), "unknown stage id fails safely");
		Check(missingResult.validation.error == StageLoadError::NotFound, "unknown stage reports NotFound");
		Check(!missingResult.stage.has_value(), "failed stage load does not return partial data");

		StageDefinition invalid = CreateDefaultStageDefinition();
		invalid.id.clear();
		Check(ValidateStageDefinition(invalid).error == StageLoadError::EmptyId, "empty stage id is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.pegLayout.pegs.clear();
		Check(ValidateStageDefinition(invalid).error == StageLoadError::EmptyPegLayout, "empty stage board is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.pegLayout.pegs[0].position = { 0.0f, 0.0f };
		Check(ValidateStageDefinition(invalid).error == StageLoadError::PegOutOfBounds, "out-of-board peg is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.pegLayout.pegs[1].position = invalid.pegLayout.pegs[0].position;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::DuplicatePegPosition, "duplicate peg position is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.rules.enemyHealth = 0.0f;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemyHealth, "non-positive enemy health is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.rules.enemyStepsBeforeAttack = 0;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemySteps, "non-positive attack timing is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.rules.pegRestitution = 2.0f;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidPegRestitution, "out-of-range stage restitution is rejected");
	}

	void TestStageRulesConfigureWorld()
	{
		StageDefinition stage = CreateDefaultStageDefinition();
		stage.id = "test-stage";
		stage.displayName = "Test Stage";
		stage.pegLayout.pegs = { { { 500.0f, 500.0f }, PegType::Normal } };
		stage.rules.playerHealth = 75.0f;
		stage.rules.enemyHealth = 9.0f;
		stage.rules.playerDamage = 15.0f;
		stage.rules.enemyStepsBeforeAttack = 2;
		stage.rules.enemyStep = 10.0f;
		stage.rules.pegRestitution = 0.5f;
		Check(ValidateStageDefinition(stage).IsValid(), "custom stage rules validate");

		GameWorld world(stage);
		Check(world.GetStage().id == "test-stage", "game world retains loaded stage identity");
		Check(world.GetTargets()._targetBallList.GetCount() == 1, "stage board configures game targets");
		Check(Near(world.GetPlayer().GetHp(), 75.0f), "stage configures player health");
		Check(Near(world.GetEnemy().GetHp(), 9.0f), "stage configures enemy health");
		Check(Near(world.GetPegRestitution(), 0.5f), "stage configures peg restitution");

		const float initialEnemyX = world.GetEnemy().GetX();
		Launch(world);
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		Check(Near(world.GetEnemy().GetX(), initialEnemyX - 10.0f), "stage configures enemy movement step");
		Check(Near(world.GetPlayer().GetHp(), 75.0f), "enemy movement turn does not damage player early");

		world.GetEnemy().SetCount(2);
		Launch(world);
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		Check(Near(world.GetPlayer().GetHp(), 60.0f), "stage configures player damage per enemy attack");

		world.SetPegRestitution(1.0f);
		world.ResetGame();
		Check(Near(world.GetPlayer().GetHp(), 75.0f), "stage reset restores configured player health");
		Check(Near(world.GetEnemy().GetHp(), 9.0f), "stage reset restores configured enemy health");
		Check(Near(world.GetPegRestitution(), 0.5f), "stage reset restores configured restitution");
	}
}

int main()
{
	TestZeroLengthShot();
	TestWallReflection();
	TestPegReflectionAndSingleDamage();
	TestScoreComboProgression();
	TestScreenExitTurn();
	TestVictoryTransition();
	TestDefeatTransition();
	TestStateAndRestitutionRules();
	TestLayoutConfiguration();
	TestSeededPegLayout();
	TestDataDrivenPegLayout();
	TestPegTypeDefinitions();
	TestCriticalPegEffect();
	TestBombPegEffect();
	TestRefreshPegEffect();
	TestStageCatalogAndValidation();
	TestStageRulesConfigureWorld();

	if (failures == 0)
	{
		std::cout << "All Peglin core tests passed.\n";
		return 0;
	}

	std::cerr << failures << " Peglin core test(s) failed.\n";
	return 1;
}
