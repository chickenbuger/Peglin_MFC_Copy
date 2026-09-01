#include <afxwin.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include "GameWorld.h"
#include "ContentCatalog.h"
#include "GameLayout.h"
#include "GameRecordStore.h"
#include "GameSettingsStore.h"
#include "GameplayCatalog.h"
#include "Localization.h"
#include "PegLayout.h"
#include "Physics.h"
#include "Progression.h"
#include "RewardPresentation.h"
#include "RunProgression.h"
#include "SoftPegSound.h"
#include "TerminalTransitionGate.h"
#include "UiNavigation.h"

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

	std::uint16_t ReadWaveUInt16(
		const std::vector<std::uint8_t>& wave,
		std::size_t offset)
	{
		return static_cast<std::uint16_t>(wave[offset])
			| static_cast<std::uint16_t>(wave[offset + 1]) << 8U;
	}

	std::uint32_t ReadWaveUInt32(
		const std::vector<std::uint8_t>& wave,
		std::size_t offset)
	{
		std::uint32_t value = 0;
		for (std::size_t index = 0; index < 4; ++index)
		{
			value |= static_cast<std::uint32_t>(wave[offset + index])
				<< static_cast<unsigned int>(index * 8U);
		}
		return value;
	}

	void TestSoftPegHitSound()
	{
		const std::vector<std::uint8_t> wave = CreateSoftPegHitWave();
		Check(wave.size() > 44U, "soft peg sound contains PCM samples");
		Check(wave[0] == 'R' && wave[1] == 'I' && wave[2] == 'F' && wave[3] == 'F',
			"soft peg sound has a RIFF header");
		Check(wave[8] == 'W' && wave[9] == 'A' && wave[10] == 'V' && wave[11] == 'E',
			"soft peg sound has a WAVE signature");
		Check(ReadWaveUInt16(wave, 20) == 1U, "soft peg sound uses PCM encoding");
		Check(ReadWaveUInt16(wave, 22) == 1U, "soft peg sound is mono");
		Check(ReadWaveUInt32(wave, 24) == 22050U, "soft peg sound uses a compact sample rate");
		Check(ReadWaveUInt16(wave, 34) == 16U, "soft peg sound uses sixteen-bit samples");
		Check(ReadWaveUInt32(wave, 40) + 44U == wave.size(),
			"soft peg sound data length matches its header");

		int peak = 0;
		for (std::size_t offset = 44; offset + 1 < wave.size(); offset += 2)
		{
			const auto sample = static_cast<std::int16_t>(ReadWaveUInt16(wave, offset));
			peak = (std::max)(peak, std::abs(static_cast<int>(sample)));
		}
		Check(peak >= 1000 && peak <= 5000, "soft peg sound keeps a gentle audible peak");
		Check(ReadWaveUInt16(wave, 44) == 0U, "soft peg sound begins without a click");
		Check(ReadWaveUInt16(wave, wave.size() - 2U) == 0U,
			"soft peg sound fades to silence");
	}

	void Launch(GameWorld& world)
	{
		Check(world.BeginAim({ 100.0f, 100.0f }), "begin aim");
		Check(world.ReleaseShot({ 90.0f, 100.0f }), "launch non-zero shot");
		Check(world.GetState() == GameState::BallInFlight, "shot enters BallInFlight");
	}

	int CountActivePegsOfType(const GameWorld& world, PegType type)
	{
		int count = 0;
		auto position = world.GetTargets()._targetBallList.GetHeadPosition();
		while (position != nullptr)
		{
			const TargetBall& peg = world.GetTargets()._targetBallList.GetNext(position);
			count += peg.type == type ? 1 : 0;
		}
		return count;
	}

	std::vector<std::size_t> GetActivePegSourcesOfType(
		const GameWorld& world,
		PegType type)
	{
		std::vector<std::size_t> sourceIndices;
		auto position = world.GetTargets()._targetBallList.GetHeadPosition();
		while (position != nullptr)
		{
			const TargetBall& peg = world.GetTargets()._targetBallList.GetNext(position);
			if (peg.type == type)
			{
				sourceIndices.push_back(peg.GetSourceIndex());
			}
		}
		std::sort(sourceIndices.begin(), sourceIndices.end());
		return sourceIndices;
	}

	int CountMovingPegs(const StageDefinition& stage)
	{
		return static_cast<int>(std::count_if(
			stage.pegLayout.pegs.begin(),
			stage.pegLayout.pegs.end(),
			[](const PegDefinition& peg) { return peg.motion.IsMoving(); }));
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

	void TestTerminalTransitionGate()
	{
		TerminalTransitionGate gate;
		Check(!gate.IsPending(), "terminal transition gate starts idle");
		Check(gate.CompleteIfReady(true, true) == GameUpdateResult::None,
			"idle terminal transition gate has no result");

		gate.Queue(GameUpdateResult::Victory);
		Check(gate.IsPending(), "victory queues a terminal transition");
		Check(gate.CompleteIfReady(false, true) == GameUpdateResult::None,
			"attack animation delays the terminal transition");
		Check(gate.CompleteIfReady(true, false) == GameUpdateResult::None,
			"feedback animation delays the terminal transition");
		gate.Queue(GameUpdateResult::Defeat);
		Check(gate.CompleteIfReady(true, true) == GameUpdateResult::Victory,
			"queued terminal transition completes with the original result");
		Check(!gate.IsPending(), "completed terminal transition returns to idle");
		Check(gate.CompleteIfReady(true, true) == GameUpdateResult::None,
			"completed terminal transition is reported only once");

		gate.Queue(GameUpdateResult::Defeat);
		gate.Reset();
		Check(!gate.IsPending(), "terminal transition reset cancels pending state");
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
			if (candidate.type == PegType::Normal
				&& candidate.position.x > GameLayout::PegFieldLeft
				&& candidate.position.x < GameLayout::PegFieldRight)
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
		StageDefinition stage = CreateDefaultStageDefinition();
		stage.id = "close-range-defeat";
		stage.rules.enemyStepsBeforeAttack = 1;
		GameWorld world(stage);
		world.GetPlayer().SetHp(20.0f);
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

	void TestMouseUiNavigation()
	{
		UiAction action = ResolveUiClick(
			UiScreenKind::StageSelection,
			{ 790.0f, 400.0f },
			2);
		Check(action.command == UiCommand::SelectStage && action.index == 1, "mouse selects the second route card");
		Check(
			ResolveUiClick(UiScreenKind::StageSelection, { 600.0f, 665.0f }, 3).command
				== UiCommand::StartSelectedStage,
			"mouse starts the selected stage");
		Check(
			ResolveUiClick(UiScreenKind::StageSelection, { 100.0f, 600.0f }, 3).command
				== UiCommand::OpenLoadout,
			"mouse opens loadout management");
		Check(
			ResolveUiClick(UiScreenKind::StageSelection, { 100.0f, 665.0f }, 3).command
				== UiCommand::OpenOptions,
			"mouse opens options");

		action = ResolveUiClick(UiScreenKind::Loadout, { 400.0f, 250.0f }, 0);
		Check(action.command == UiCommand::SelectOrb && action.index == 1, "mouse selects an orb card");
		action = ResolveUiClick(UiScreenKind::Loadout, { 705.0f, 450.0f }, 0);
		Check(action.command == UiCommand::AcquireRelic && action.index == 2, "mouse acquires a relic card");
		Check(
			ResolveUiClick(UiScreenKind::Loadout, { 150.0f, 665.0f }, 0).command
				== UiCommand::ResetProgression,
			"mouse resets the loadout");
		Check(
			ResolveUiClick(UiScreenKind::Loadout, { 800.0f, 665.0f }, 0).command
				== UiCommand::BackToStageSelection,
			"mouse returns from loadout");

		Check(
			ResolveUiClick(UiScreenKind::Options, { 500.0f, 245.0f }, 0).command
				== UiCommand::ToggleDifficulty,
			"mouse changes difficulty");
		Check(
			ResolveUiClick(UiScreenKind::Options, { 500.0f, 355.0f }, 0).command
				== UiCommand::ToggleSound,
			"mouse toggles sound");
		Check(
			ResolveUiClick(UiScreenKind::Options, { 500.0f, 465.0f }, 0).command
				== UiCommand::TogglePegColorMode,
			"mouse changes peg accessibility mode");
		Check(
			ResolveUiClick(UiScreenKind::Options, { 500.0f, 560.0f }, 0).command
				== UiCommand::ToggleLanguage,
			"mouse changes the UI language");
		Check(
			ResolveUiClick(UiScreenKind::Result, { 350.0f, 665.0f }, 0).command
				== UiCommand::RetryStage,
			"mouse retries from result screen");
		Check(
			!ResolveUiClick(UiScreenKind::StageSelection, { 10.0f, 10.0f }, 3).IsHandled(),
			"mouse ignores non-interactive background");
		action = ResolveUiClick(UiScreenKind::Reward, { 685.0f, 360.0f }, 0);
		Check(action.command == UiCommand::SelectReward && action.index == 2,
			"mouse selects the clicked run reward card");
		Check(
			!ResolveUiClick(UiScreenKind::Reward, { 500.0f, 600.0f }, 0).IsHandled(),
			"reward screen ignores non-interactive background");
		action = ResolveUiClick(UiScreenKind::Shop, { 330.0f, 300.0f }, 0);
		Check(action.command == UiCommand::BuyShopOffer && action.index == 0,
			"mouse buys the clicked shop offer");
		Check(
			ResolveUiClick(UiScreenKind::Shop, { 500.0f, 665.0f }, 0).command
				== UiCommand::LeaveShop,
			"mouse leaves the shop from the continue button");
		Check(
			!ResolveUiClick(UiScreenKind::Shop, { 20.0f, 20.0f }, 0).IsHandled(),
			"shop screen ignores non-interactive background");
	}

	void TestAdventureRunProgression()
	{
		const std::vector<RunStageEntry> stageCatalog{
			{ "stage-1", false },
			{ "stage-2", false },
			{ "stage-4", false },
			{ "stage-5", false },
			{ "stage-6", false },
			{ "stage-7", false },
			{ "stage-8", false },
			{ "stage-3", true }
		};
		const RunStageLayers route = BuildBranchingStageLayers(stageCatalog);
		Check(route.size() == 6, "branching route creates four encounters, a shop, and a boss layer");
		Check(route[0].size() == 1 && route[0][0] == "stage-1", "branching route keeps one fixed opening stage");
		Check(route[1].size() == 2 && route[1][0] == "stage-2" && route[1][1] == "stage-4",
			"first cleared stage opens two route choices");
		Check(route[2].size() == 2 && route[4].size() == 2, "combat route layers each expose two choices");
		Check(route[3].size() == 1 && IsRunShopStage(route[3][0]),
			"branching route inserts one fixed shop node before the final branch");
		Check(route[5].size() == 1 && route[5][0] == "stage-3", "branching route always ends at the boss");
		Check(BuildBranchingStageLayers({ { "only", false }, { "boss", true } }).empty(),
			"route builder rejects an undersized catalog");
		Check(BuildBranchingStageLayers({ { "a", false }, { "b", false }, { "boss-a", true }, { "boss-b", true } }).empty(),
			"route builder rejects multiple bosses");
		Check(BuildBranchingStageLayers({ { "same", false }, { "same", false }, { "boss", true } }).empty(),
			"route builder rejects duplicate stage ids");
		Check(BuildBranchingStageLayers({ { std::string(RunShopStageId), false }, { "other", false }, { "boss", true } }).empty(),
			"route builder reserves the shop node id");
		const auto& shopOffers = GetRunShopOffers();
		Check(shopOffers.size() == 3, "shop exposes three deterministic offers");
		Check(shopOffers[0].reward.kind == RunRewardKind::Orb && shopOffers[0].price == 45,
			"shop offers a priced orb");
		Check(shopOffers[1].reward.kind == RunRewardKind::Relic && shopOffers[1].price == 70,
			"shop offers a priced relic");
		Check(shopOffers[2].reward.kind == RunRewardKind::Heal && shopOffers[2].price == 30,
			"shop offers priced healing");

		AdventureRun run;
		Check(!run.Start({}), "run rejects an empty stage path");
		Check(run.StartBranching(route), "run accepts a validated branching route");
		Check(run.GetStatus() == RunStatus::StageReady, "new run begins at a ready stage");
		Check(run.GetCurrentStageId() == "stage-1", "new run begins at the first stage");
		Check(run.GetClearedStageCount() == 0, "new run begins with no cleared stages");
		Check(run.GetGold() == 50, "new run begins with starter gold");

		Check(run.CompleteCurrentStage(), "first stage victory advances the run");
		Check(run.GetGold() == 75, "combat victory awards gold");
		Check(run.GetStatus() == RunStatus::RewardSelection, "non-final victory requires a reward");
		Check(run.GetRewardChoices().size() == 3, "stage victory offers three rewards");
		Check(run.GetRewardChoices()[0].kind == RunRewardKind::Orb, "reward set includes an orb");
		Check(run.GetRewardChoices()[1].kind == RunRewardKind::Relic, "reward set includes a relic");
		Check(run.GetRewardChoices()[2].kind == RunRewardKind::Heal, "reward set includes healing");
		Check(!run.CompleteCurrentStage(), "run cannot skip the required reward");
		Check(!run.SelectReward(3).has_value(), "run rejects an invalid reward index");
		const auto firstReward = run.SelectReward(0);
		Check(firstReward.has_value() && firstReward->id == "iron-orb", "run returns the selected reward");
		Check(run.GetStatus() == RunStatus::StageChoice, "reward opens the next route choice");
		Check(run.GetAvailableStageIds().size() == 2, "first route choice exposes two stages");
		Check(!run.GetSelectedStageChoiceIndex().has_value(),
			"route choice starts without an implicit selection");
		Check(!run.ConfirmSelectedStage(),
			"route cannot be confirmed before a stage is selected");
		Check(!run.SelectNextStage(2), "run rejects an invalid route choice");
		Check(run.SelectNextStage(1), "run accepts the selected branch");
		Check(run.GetStatus() == RunStatus::StageChoice,
			"selecting a route keeps both choices editable until start");
		Check(run.GetAvailableStageIds().size() == 2,
			"selecting a route preserves every visible candidate");
		Check(run.GetSelectedStageChoiceIndex() == std::optional<std::size_t>{ 1 }
			&& run.GetSelectedStageChoiceId() == "stage-4",
			"route selection records the highlighted candidate");
		Check(run.GetCurrentStageId() == "stage-1",
			"unconfirmed route does not replace the current stage");
		Check(!run.CompleteCurrentStage(),
			"unconfirmed route cannot advance stage progress");
		Check(run.SelectNextStage(0)
			&& run.GetSelectedStageChoiceId() == "stage-2",
			"route selection can change to the other candidate before start");
		Check(run.SelectNextStage(1)
			&& run.GetSelectedStageChoiceId() == "stage-4",
			"route selection can change back before start");
		Check(run.ConfirmSelectedStage(),
			"start confirmation commits the highlighted route");
		Check(run.GetStatus() == RunStatus::StageReady
			&& run.GetAvailableStageIds().empty()
			&& !run.GetSelectedStageChoiceIndex().has_value(),
			"confirmed route becomes ready and clears temporary choice state");
		Check(run.GetCurrentStageId() == "stage-4", "selected branch becomes the current stage");

		Check(run.CompleteCurrentStage(), "second encounter victory advances the run");
		Check(run.GetGold() == 100, "second combat victory awards gold");
		Check(run.SelectReward(2).has_value(), "second encounter reward can be selected");
		Check(run.SelectNextStage(0), "run accepts a stage from the second branch layer");
		Check(run.ConfirmSelectedStage(), "second branch selection is confirmed at start");
		Check(run.GetCurrentStageId() == "stage-5", "second branch selection is retained");
		Check(run.CompleteCurrentStage(), "third encounter victory advances the run");
		Check(run.GetGold() == 125, "third combat victory funds the shop");
		Check(run.SelectReward(1).has_value(), "third encounter reward can be selected");
		Check(run.GetAvailableStageIds().size() == 1
			&& IsRunShopStage(run.GetAvailableStageIds()[0]),
			"third encounter opens the fixed shop node");
		Check(run.SelectNextStage(0) && run.ConfirmSelectedStage(),
			"shop node is selected and confirmed like a stage");
		Check(IsRunShopStage(run.GetCurrentStageId()), "confirmed shop becomes the current run node");
		Check(!run.CompleteCurrentStage(), "shop cannot grant a combat reward");
		Check(!run.SpendGold(0) && !run.SpendGold(126),
			"shop wallet rejects invalid and unaffordable charges");
		Check(run.SpendGold(45) && run.GetGold() == 80, "shop wallet deducts an affordable purchase");
		Check(run.CompleteCurrentStage(false), "leaving the shop advances without a reward screen");
		Check(run.GetStatus() == RunStatus::StageChoice
			&& run.GetAvailableStageIds().size() == 2,
			"shop exit opens the final combat branch directly");
		Check(run.SelectNextStage(1), "run accepts a stage from the final branch layer");
		Check(run.ConfirmSelectedStage(), "final branch selection is confirmed at start");
		Check(run.GetCurrentStageId() == "stage-8", "final branch selection is retained");
		Check(run.CompleteCurrentStage(), "fourth encounter victory advances to the boss route");
		Check(run.SelectReward(2).has_value(), "pre-boss reward can be selected");
		Check(run.GetAvailableStageIds().size() == 1 && run.GetAvailableStageIds()[0] == "stage-3",
			"pre-boss route exposes only the boss");
		Check(run.SelectNextStage(0), "boss route can be selected");
		Check(run.ConfirmSelectedStage(), "boss route is confirmed at start");
		Check(run.GetCurrentStageId() == "stage-3", "selected boss becomes the current stage");
		run.MarkDefeated();
		Check(run.GetStatus() == RunStatus::Defeated, "defeat marks the run failed");
		Check(run.RetryCurrentStage(), "failed run can retry the current stage");
		Check(run.GetStatus() == RunStatus::StageReady, "retry restores stage-ready state");
		Check(run.CompleteCurrentStage(), "boss victory completes the final stage");
		Check(run.GetStatus() == RunStatus::Complete, "final victory completes the run");
		Check(run.GetClearedStageCount() == 6, "completed run records combat and shop visits");
		Check(run.GetCompletedCombatStageCount() == 5, "shop visit is excluded from combat clear totals");
		Check(run.GetGold() == 130, "only combat victories add gold after shop spending");
		Check(run.HasClearedStage(RunShopStageId), "run history records the visited shop node");
		Check(run.HasClearedStage("stage-4") && !run.HasClearedStage("stage-2"),
			"run history records the chosen branch without marking skipped stages");
		Check(run.GetRewardChoices().empty(), "final victory does not offer a next-stage reward");
	}

	void TestLayoutConfiguration()
	{
		GameWorld world;
		Check(Near(world.GetBall().GetPosition().x, GameLayout::BallInitialPosition.x), "layout controls initial ball x");
		Check(Near(world.GetBall().GetPosition().y, GameLayout::BallInitialPosition.y), "layout controls initial ball y");
		Check(
			world.GetBall().GetPosition().x >= GameLayout::PegFieldLeft + GameLayout::BallRadius
				&& world.GetBall().GetPosition().x <= GameLayout::PegFieldRight - GameLayout::BallRadius
				&& world.GetBall().GetPosition().y >= GameLayout::PegFieldTop + GameLayout::BallRadius
				&& world.GetBall().GetPosition().y <= GameLayout::PegFieldBottom - GameLayout::BallRadius,
			"initial ball begins inside displayed peg field");
		Check(
			GameLayout::BallTopBoundary >= GameLayout::PegFieldTop + GameLayout::BallRadius,
			"ball ceiling remains inside displayed peg field");
		Check(
			GameLayout::PegFieldRight - GameLayout::PegFieldLeft <= 620.0f,
			"peg field uses the compact gameplay width");
		Check(
			GameLayout::OrbHudLeft > GameLayout::PegFieldRight
			&& GameLayout::OrbHudRight <= GameLayout::WindowWidth,
			"orb queue occupies a dedicated side region outside the peg field");
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
				&& position.x >= GameLayout::PegFieldLeft + GameLayout::PegRadius
				&& position.x <= GameLayout::PegFieldRight - GameLayout::PegRadius
				&& position.y >= GameLayout::PegFieldTop + GameLayout::PegRadius
				&& position.y <= GameLayout::PegFieldBottom - GameLayout::PegRadius;
		}

		Check(sameSeedMatches, "same seed reproduces identical peg positions");
		Check(differentSeedDiffers, "different seed changes peg positions");
		Check(allInsideBoard, "seeded jitter keeps every peg inside the displayed field");
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

	void TestMovingPegStages()
	{
		constexpr float PI = 3.1415927f;
		PegLayoutDefinition layout;
		layout.pegs = {
			{
				{ 500.0f, 500.0f },
				PegType::Normal,
				{ PegMotionKind::Horizontal, 20.0f, PI, 0.0f }
			},
			{ { 600.0f, 500.0f }, PegType::Refresh }
		};
		GameWorld world(layout);
		auto movingPosition = world.GetTargets()._targetBallList.GetHeadPosition();
		const TargetBall& initial = world.GetTargets()._targetBallList.GetAt(movingPosition);
		Check(initial.IsMoving() && Near(initial.position.x, 500.0f),
			"moving peg starts from its deterministic phase position");

		world.Update(0.5f);
		movingPosition = world.GetTargets()._targetBallList.GetHeadPosition();
		const TargetBall& shifted = world.GetTargets()._targetBallList.GetAt(movingPosition);
		Check(Near(shifted.position.x, 520.0f) && Near(shifted.position.y, 500.0f),
			"horizontal peg follows its configured sinusoidal path while aiming");

		Check(world.TogglePause(), "moving peg test enters pause");
		world.Update(0.5f);
		movingPosition = world.GetTargets()._targetBallList.GetHeadPosition();
		Check(Near(world.GetTargets()._targetBallList.GetAt(movingPosition).position.x, 520.0f),
			"paused stage freezes moving pegs");
		Check(world.TogglePause(), "moving peg test resumes from pause");
		world.Update(0.5f);
		movingPosition = world.GetTargets()._targetBallList.GetHeadPosition();
		Check(Near(world.GetTargets()._targetBallList.GetAt(movingPosition).position.x, 500.0f),
			"resumed moving peg continues from the preserved phase");

		world.ResetGame();
		movingPosition = world.GetTargets()._targetBallList.GetHeadPosition();
		Check(Near(world.GetTargets()._targetBallList.GetAt(movingPosition).position.x, 500.0f),
			"stage retry resets moving pegs to their initial phase");
		world.Update(0.5f);
		Launch(world);
		world.GetBall().SetPosition({ 505.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		Check(world.GetTargets()._targetBallList.GetCount() == 1,
			"ball collision uses the moving peg's current position");

		GameWorld refreshWorld(layout);
		refreshWorld.Update(0.5f);
		Launch(refreshWorld);
		refreshWorld.GetBall().SetPosition({ 585.0f, 500.0f });
		refreshWorld.GetBall().SetVelocity({ 2.0f, 0.0f });
		refreshWorld.Update(0.0f);
		Check(refreshWorld.GetTargets()._targetBallList.GetCount() == 2,
			"refresh identifies an active moving peg by source index without duplicating it");
		int movingCount = 0;
		auto scan = refreshWorld.GetTargets()._targetBallList.GetHeadPosition();
		while (scan != nullptr)
		{
			const TargetBall& target = refreshWorld.GetTargets()._targetBallList.GetNext(scan);
			movingCount += target.IsMoving() ? 1 : 0;
		}
		Check(movingCount == 1, "refresh preserves exactly one restored moving peg definition");
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
		Check(world.GetTargets()._targetBallList.GetCount() == 3, "refresh restores removed pegs and replaces itself");
		Check(world.GetFeedback().currentShotPegHits == 2, "refresh collision counts as one additional hit");
		Check(world.GetScore().currentShot == 300, "refresh hit preserves normal combo scoring");

		bool restoredNormalFound = false;
		int activeRefreshPegs = 0;
		auto position = world.GetTargets()._targetBallList.GetHeadPosition();
		while (position != nullptr)
		{
			const TargetBall& target = world.GetTargets()._targetBallList.GetNext(position);
			if (Near(target.position.x, 300.0f) && target.type == PegType::Normal)
			{
				restoredNormalFound = true;
			}
			activeRefreshPegs += target.type == PegType::Refresh ? 1 : 0;
		}
		Check(restoredNormalFound, "refresh restores the removed peg with its original type");
		Check(activeRefreshPegs == 1, "refresh activation always adds one replacement refresh peg");
		const std::vector<std::size_t> relocatedSources =
			GetActivePegSourcesOfType(world, PegType::Refresh);
		Check(relocatedSources.size() == 1 && relocatedSources.front() != 1,
			"destroyed refresh peg respawns at a different source position");
		const std::vector<GameEvent> refreshEvents = world.ConsumeEvents();
		Check(std::any_of(refreshEvents.begin(), refreshEvents.end(), [](const GameEvent& event)
			{
				return event.type == GameEventType::RefreshRelocated;
			}), "refresh destruction emits relocation feedback");
	}

	void TestRefreshPegMinimumAfterTurn()
	{
		PegLayoutDefinition layout;
		layout.pegs = {
			{ { 300.0f, 500.0f }, PegType::Normal },
			{ { 500.0f, 500.0f }, PegType::Critical }
		};
		GameWorld world(layout);
		Launch(world);
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		world.Update(0.0f);

		Check(CountActivePegsOfType(world, PegType::Refresh) == 1,
			"turn completion converts one active peg when refresh is missing");
		const std::vector<GameEvent> events = world.ConsumeEvents();
		Check(std::any_of(events.begin(), events.end(), [](const GameEvent& event)
			{
				return event.type == GameEventType::RefreshGuaranteed
					&& event.affectedPegs == 1;
			}), "refresh guarantee emits one deterministic replacement event");

		PegLayoutDefinition emptyAfterShotLayout;
		emptyAfterShotLayout.pegs = {
			{ { 500.0f, 500.0f }, PegType::Normal }
		};
		GameWorld emptyAfterShot(emptyAfterShotLayout);
		Launch(emptyAfterShot);
		emptyAfterShot.GetBall().SetPosition({ 485.0f, 500.0f });
		emptyAfterShot.GetBall().SetVelocity({ 2.0f, 0.0f });
		emptyAfterShot.Update(0.0f);
		emptyAfterShot.GetBall().SetPosition({ 500.0f, 801.0f });
		emptyAfterShot.Update(0.0f);
		emptyAfterShot.Update(0.0f);
		Check(emptyAfterShot.GetTargets()._targetBallList.GetCount() == 1
			&& CountActivePegsOfType(emptyAfterShot, PegType::Refresh) == 1,
			"turn completion recreates a refresh peg when the board is empty");

		PegLayoutDefinition existingRefreshLayout;
		existingRefreshLayout.pegs = {
			{ { 300.0f, 500.0f }, PegType::Refresh },
			{ { 500.0f, 500.0f }, PegType::Normal }
		};
		GameWorld existingRefresh(existingRefreshLayout);
		Launch(existingRefresh);
		existingRefresh.GetBall().SetPosition({ 500.0f, 801.0f });
		existingRefresh.Update(0.0f);
		existingRefresh.Update(0.0f);
		const std::vector<std::size_t> firstTurnRefreshSources =
			GetActivePegSourcesOfType(existingRefresh, PegType::Refresh);
		const std::vector<GameEvent> existingEvents = existingRefresh.ConsumeEvents();
		Check(CountActivePegsOfType(existingRefresh, PegType::Refresh) == 1,
			"turn completion preserves the configured refresh peg count");
		Check(firstTurnRefreshSources.size() == 1 && firstTurnRefreshSources.front() != 0,
			"turn completion moves an existing refresh peg to another source");
		Check(std::none_of(existingEvents.begin(), existingEvents.end(), [](const GameEvent& event)
			{
				return event.type == GameEventType::RefreshGuaranteed;
			}), "existing refresh peg avoids an unnecessary guarantee event");
		Check(std::any_of(existingEvents.begin(), existingEvents.end(), [](const GameEvent& event)
			{
				return event.type == GameEventType::RefreshRelocated;
			}), "turn completion emits refresh relocation feedback");

		Launch(existingRefresh);
		existingRefresh.GetBall().SetPosition({ 500.0f, 801.0f });
		existingRefresh.Update(0.0f);
		existingRefresh.Update(0.0f);
		const std::vector<std::size_t> secondTurnRefreshSources =
			GetActivePegSourcesOfType(existingRefresh, PegType::Refresh);
		Check(secondTurnRefreshSources.size() == 1
			&& secondTurnRefreshSources != firstTurnRefreshSources,
			"refresh peg changes source position on every completed turn");

		PegLayoutDefinition blastedRefreshLayout;
		blastedRefreshLayout.pegs = {
			{ { 500.0f, 500.0f }, PegType::Bomb },
			{ { 580.0f, 500.0f }, PegType::Refresh },
			{ { 720.0f, 500.0f }, PegType::Normal }
		};
		GameWorld blastedRefresh(blastedRefreshLayout);
		Launch(blastedRefresh);
		blastedRefresh.GetBall().SetPosition({ 485.0f, 500.0f });
		blastedRefresh.GetBall().SetVelocity({ 2.0f, 0.0f });
		blastedRefresh.Update(0.0f);
		const std::vector<std::size_t> blastedRefreshSources =
			GetActivePegSourcesOfType(blastedRefresh, PegType::Refresh);
		Check(blastedRefreshSources.size() == 1 && blastedRefreshSources.front() != 1,
			"blast-destroyed refresh peg immediately respawns at a different source");
	}

	void TestStageCatalogAndValidation()
	{
		const auto& catalog = GetStageCatalog();
		Check(catalog.size() == 8, "stage catalog exposes eight route stages");
		Check(
			catalog[0].id == "stage-1"
			&& catalog[1].id == "stage-2"
			&& catalog[2].id == "stage-4"
			&& catalog[7].id == "stage-3",
			"stage catalog keeps stable selection ids");

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
		Check(CountMovingPegs(*defaultResult.stage) == 0,
			"opening stage keeps a static introductory peg board");
		Check(CountMovingPegs(*challengeResult.stage) == 3,
			"dense cavern introduces three moving pegs");
		for (std::string_view stageId : { "stage-4", "stage-5", "stage-6", "stage-7", "stage-8" })
		{
			const StageLoadResult added = LoadStageDefinition(stageId);
			Check(added.IsSuccess(), "added route stage loads and validates");
			Check(added.stage.has_value() && added.stage->enemies.size() == 3,
				"added route stage provides a three-monster encounter");
		}
		const StageLoadResult thornwood = LoadStageDefinition("stage-4");
		Check(
			thornwood.stage.has_value()
			&& thornwood.stage->enemies[0].visual == EnemyVisualKind::ThornbackWolf,
			"built-in route includes the thornback wolf visual");
		const StageLoadResult fungal = LoadStageDefinition("stage-5");
		Check(
			fungal.stage.has_value()
			&& fungal.stage->enemies[1].visual == EnemyVisualKind::AzureWisp,
			"built-in route includes the azure wisp visual");

		const StageLoadResult bossResult = LoadStageDefinition("stage-3");
		Check(bossResult.IsSuccess(), "stage catalog loads stage-3");
		Check(bossResult.stage->isBoss, "stage-3 is marked as a boss stage");
		Check(bossResult.stage->pegLayout.pegs.size() == 36, "boss stage provides its own board");
		Check(bossResult.stage->enemyPattern.size() == 4, "boss stage exposes a four-action pattern");
		Check(Near(bossResult.stage->rules.enemyHealth, 60.0f), "boss stage provides boss health");
		Check(CountMovingPegs(*fungal.stage) == 3,
			"fungal route contains a vertical moving-peg pattern");
		Check(CountMovingPegs(*bossResult.stage) == 4,
			"boss board contains a mixed moving-peg pattern");

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

		invalid = CreateDefaultStageDefinition();
		invalid.pegLayout.pegs[0].motion = {
			PegMotionKind::Horizontal, 0.0f, 1.0f, 0.0f };
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidPegMotion,
			"moving peg rejects a non-positive amplitude");

		invalid = CreateDefaultStageDefinition();
		invalid.pegLayout.pegs[0].motion = {
			PegMotionKind::Horizontal, 20.0f, 1.0f, 0.0f };
		Check(ValidateStageDefinition(invalid).error == StageLoadError::PegMotionOutOfBounds,
			"moving peg path must remain inside the peg field");
	}

	void TestStageSelectionAndResultSummary()
	{
		GameWorld world;
		Check(!world.GetResultSummary().has_value(), "active stage has no result summary");
		Check(!world.LoadStage("missing-stage"), "stage selection rejects unregistered id");
		Check(world.GetStage().id == "stage-1", "failed selection preserves current stage");
		Check(world.LoadStage("stage-2"), "stage selection loads registered challenge stage");
		Check(world.GetStage().id == "stage-2", "selected stage identity becomes active");
		Check(world.GetTargets()._targetBallList.GetCount() == 40, "selected challenge stage creates forty pegs");
		Check(Near(world.GetEnemy().GetHp(), 30.0f), "selected challenge stage applies enemy health");
		world.ResetGame();
		Check(world.GetStage().id == "stage-2", "retry preserves selected stage");

		StageDefinition resultStage = CreateDefaultStageDefinition();
		resultStage.id = "result-stage";
		resultStage.displayName = "Result Stage";
		resultStage.pegLayout.pegs = { { { 500.0f, 500.0f }, PegType::Critical } };
		resultStage.rules.enemyHealth = 2.0f;
		GameWorld resultWorld(resultStage);
		Launch(resultWorld);
		resultWorld.GetBall().SetPosition({ 485.0f, 500.0f });
		resultWorld.GetBall().SetVelocity({ 2.0f, 0.0f });
		resultWorld.Update(0.0f);
		resultWorld.GetBall().SetPosition({ 500.0f, 801.0f });
		resultWorld.Update(0.0f);
		Check(resultWorld.Update(0.0f) == GameUpdateResult::Victory, "result stage reaches victory");
		const std::optional<GameResultSummary> summary = resultWorld.GetResultSummary();
		Check(summary.has_value(), "terminal stage exposes result summary");
		Check(summary.has_value() && summary->result == GameUpdateResult::Victory, "result summary preserves outcome");
		Check(summary.has_value() && summary->stageId == "result-stage", "result summary preserves stage identity");
		Check(summary.has_value() && summary->totalScore == 200, "result summary preserves final score");
		Check(summary.has_value() && summary->bestCombo == 1, "result summary preserves best combo");
		Check(summary.has_value() && summary->turns == 1, "result summary preserves completed turns");
		Check(resultWorld.Update(0.0f) == GameUpdateResult::None, "result screen source remains single-shot");
	}

	void TestGameOptionsAndDifficulty()
	{
		GameOptions options;
		Check(options.difficulty == GameDifficulty::Normal, "options default to normal difficulty");
		Check(options.soundEnabled, "options default to sound enabled");
		Check(options.showGameplayInfo, "options default to visible gameplay information");
		Check(options.pegColorMode == PegColorMode::Standard, "options default to standard peg colors");
		Check(options.language == UiLanguage::Korean, "options default to Korean UI text");
		options.CycleDifficulty();
		Check(options.difficulty == GameDifficulty::Hard, "difficulty cycles from normal to hard");
		options.CycleDifficulty();
		Check(options.difficulty == GameDifficulty::Easy, "difficulty cycles from hard to easy");
		options.CycleDifficulty();
		Check(options.difficulty == GameDifficulty::Normal, "difficulty cycles from easy to normal");
		options.ToggleSound();
		Check(!options.soundEnabled, "sound option toggles off");
		options.ToggleSound();
		Check(options.soundEnabled, "sound option toggles back on");
		options.ToggleGameplayInfo();
		Check(!options.showGameplayInfo, "gameplay information toggles off");
		options.ToggleGameplayInfo();
		Check(options.showGameplayInfo, "gameplay information toggles back on");
		options.TogglePegColorMode();
		Check(options.pegColorMode == PegColorMode::HighContrast, "peg colors toggle to high contrast");
		options.TogglePegColorMode();
		Check(options.pegColorMode == PegColorMode::Standard, "peg colors toggle back to standard");
		options.ToggleLanguage();
		Check(options.language == UiLanguage::English, "language toggles from Korean to English");
		options.ToggleLanguage();
		Check(options.language == UiLanguage::Korean, "language toggles back to Korean");

		const StageDefinition base = CreateDefaultStageDefinition();
		const StageDefinition easy = ApplyDifficulty(base, GameDifficulty::Easy);
		Check(Near(easy.rules.enemyHealth, 16.0f), "easy difficulty reduces enemy health to eighty percent");
		Check(Near(easy.rules.playerDamage, 15.0f), "easy difficulty reduces player damage to seventy-five percent");
		Check(easy.rules.enemyStepsBeforeAttack == 10, "easy difficulty adds two movement turns");
		Check(ValidateStageDefinition(easy).IsValid(), "easy stage remains valid");

		const StageDefinition hard = ApplyDifficulty(base, GameDifficulty::Hard);
		Check(Near(hard.rules.enemyHealth, 30.0f), "hard difficulty raises enemy health to one hundred fifty percent");
		Check(Near(hard.rules.playerDamage, 25.0f), "hard difficulty raises player damage to one hundred twenty-five percent");
		Check(hard.rules.enemyStepsBeforeAttack == 6, "hard difficulty removes two movement turns");
		Check(ValidateStageDefinition(hard).IsValid(), "hard stage remains valid");

		GameWorld world;
		Check(world.LoadStage("stage-1", GameDifficulty::Hard), "world loads stage with selected difficulty");
		Check(world.GetDifficulty() == GameDifficulty::Hard, "world retains selected difficulty");
		Check(Near(world.GetEnemy().GetHp(), 30.0f), "hard world applies tuned enemy health");
		world.ResetGame();
		Check(world.GetDifficulty() == GameDifficulty::Hard && Near(world.GetEnemy().GetHp(), 30.0f), "retry preserves difficulty tuning");
		Check(world.LoadStage("stage-1", GameDifficulty::Normal), "world can return to normal difficulty");
		Check(Near(world.GetEnemy().GetHp(), 20.0f), "normal reload restores base stage rules");
	}

	void TestGameSettingsPersistence()
	{
		const std::filesystem::path testDirectory =
			std::filesystem::temp_directory_path()
			/ ("PeglinMFC_SettingsStoreTests_" + std::to_string(::GetCurrentProcessId()));
		const std::filesystem::path settingsPath = testDirectory / "nested" / "settings.v1.ini";
		std::error_code cleanupError;
		std::filesystem::remove(settingsPath, cleanupError);
		std::filesystem::remove(settingsPath.wstring() + L".tmp", cleanupError);
		std::filesystem::remove(settingsPath.parent_path(), cleanupError);
		std::filesystem::remove(testDirectory, cleanupError);

		GameSettingsStore store(settingsPath);
		const SettingsLoadResult missing = store.Load();
		Check(missing.state == SettingsLoadState::Missing, "missing settings use a distinct load state");
		Check(missing.options.difficulty == GameDifficulty::Normal, "missing settings recover normal difficulty");
		Check(missing.options.soundEnabled, "missing settings recover enabled sound");
		Check(missing.options.showGameplayInfo, "missing settings recover visible gameplay information");
		Check(missing.options.pegColorMode == PegColorMode::Standard, "missing settings recover standard colors");
		Check(missing.options.language == UiLanguage::Korean, "missing settings recover Korean language");

		GameOptions saved;
		saved.difficulty = GameDifficulty::Hard;
		saved.soundEnabled = false;
		saved.showGameplayInfo = false;
		saved.pegColorMode = PegColorMode::HighContrast;
		saved.language = UiLanguage::English;
		std::string saveError;
		Check(store.Save(saved, &saveError), "settings save creates missing parent directories");
		Check(saveError.empty(), "successful settings save clears its error");
		Check(std::filesystem::exists(settingsPath), "settings save creates the versioned file");

		const SettingsLoadResult loaded = store.Load();
		Check(loaded.state == SettingsLoadState::Loaded, "valid settings load from disk");
		Check(loaded.options.difficulty == GameDifficulty::Hard, "settings preserve difficulty");
		Check(!loaded.options.soundEnabled, "settings preserve mute state");
		Check(!loaded.options.showGameplayInfo, "settings preserve hidden gameplay information");
		Check(loaded.options.pegColorMode == PegColorMode::HighContrast, "settings preserve high contrast mode");
		Check(loaded.options.language == UiLanguage::English, "settings preserve selected language");

		saved.difficulty = GameDifficulty::Easy;
		saved.soundEnabled = true;
		saved.showGameplayInfo = true;
		saved.pegColorMode = PegColorMode::Standard;
		saved.language = UiLanguage::Korean;
		Check(store.Save(saved), "settings save atomically replaces an existing file");
		const SettingsLoadResult replaced = store.Load();
		Check(replaced.options.difficulty == GameDifficulty::Easy, "replacement settings update difficulty");
		Check(replaced.options.soundEnabled, "replacement settings update sound");
		Check(replaced.options.showGameplayInfo, "replacement settings update gameplay information");
		Check(replaced.options.pegColorMode == PegColorMode::Standard, "replacement settings update colors");
		Check(replaced.options.language == UiLanguage::Korean, "replacement settings update language");

		{
			std::ofstream invalid(settingsPath, std::ios::trunc);
			invalid
				<< "peglin_settings_version=1\n"
				<< "difficulty=impossible\n"
				<< "sound_enabled=1\n"
				<< "peg_color_mode=standard\n";
		}
		const SettingsLoadResult unknownValue = store.Load();
		Check(unknownValue.state == SettingsLoadState::Invalid, "unknown settings value is rejected");
		Check(unknownValue.options.difficulty == GameDifficulty::Normal, "unknown value recovers default difficulty");
		Check(unknownValue.options.soundEnabled, "unknown value recovers default sound");
		Check(unknownValue.options.pegColorMode == PegColorMode::Standard, "unknown value recovers default colors");

		{
			std::ofstream legacyCurrent(settingsPath, std::ios::trunc);
			legacyCurrent
				<< "peglin_settings_version=1\n"
				<< "difficulty=hard\n"
				<< "sound_enabled=0\n"
				<< "peg_color_mode=high_contrast\n";
		}
		const SettingsLoadResult compatibleCurrent = store.Load();
		Check(compatibleCurrent.state == SettingsLoadState::Loaded, "older version one settings remain compatible");
		Check(compatibleCurrent.options.showGameplayInfo, "older version one settings default gameplay information on");
		Check(compatibleCurrent.options.language == UiLanguage::Korean, "older version one settings default to Korean");

		{
			std::ofstream invalidGameplayInfo(settingsPath, std::ios::trunc);
			invalidGameplayInfo
				<< "peglin_settings_version=1\n"
				<< "difficulty=normal\n"
				<< "sound_enabled=1\n"
				<< "show_gameplay_info=maybe\n"
				<< "peg_color_mode=standard\n";
		}
		Check(store.Load().state == SettingsLoadState::Invalid, "invalid gameplay information setting is rejected");

		{
			std::ofstream invalidLanguage(settingsPath, std::ios::trunc);
			invalidLanguage
				<< "peglin_settings_version=1\n"
				<< "difficulty=normal\n"
				<< "sound_enabled=1\n"
				<< "show_gameplay_info=1\n"
				<< "peg_color_mode=standard\n"
				<< "language=unknown\n";
		}
		Check(store.Load().state == SettingsLoadState::Invalid, "unknown UI language setting is rejected");

		{
			std::ofstream incompatible(settingsPath, std::ios::trunc);
			incompatible
				<< "peglin_settings_version=999\n"
				<< "difficulty=hard\n"
				<< "sound_enabled=0\n"
				<< "peg_color_mode=high_contrast\n";
		}
		const SettingsLoadResult unsupportedVersion = store.Load();
		Check(unsupportedVersion.state == SettingsLoadState::Invalid, "unsupported settings version is rejected");
		Check(unsupportedVersion.options.difficulty == GameDifficulty::Normal, "unsupported version recovers defaults");
		{
			std::ofstream legacy(settingsPath, std::ios::trunc);
			legacy
				<< "peglin_settings_version=0\n"
				<< "difficulty=2\n"
				<< "sound_enabled=false\n"
				<< "peg_color_mode=1\n";
		}
		const SettingsLoadResult migrated = store.Load();
		Check(migrated.state == SettingsLoadState::Migrated, "legacy settings report migration state");
		Check(migrated.options.difficulty == GameDifficulty::Hard, "legacy numeric difficulty migrates");
		Check(!migrated.options.soundEnabled, "legacy boolean sound migrates");
		Check(migrated.options.pegColorMode == PegColorMode::HighContrast, "legacy numeric color mode migrates");
		Check(store.Save(migrated.options), "migrated settings rewrite as version one");
		Check(store.Load().state == SettingsLoadState::Loaded, "rewritten settings load as current version");

		{
			std::ofstream incomplete(settingsPath, std::ios::trunc);
			incomplete << "peglin_settings_version=1\ndifficulty=hard\n";
		}
		const SettingsLoadResult incomplete = store.Load();
		Check(incomplete.state == SettingsLoadState::Invalid, "incomplete settings file is rejected");
		Check(incomplete.options.soundEnabled, "incomplete settings recover defaults");
		{
			std::ofstream duplicate(settingsPath, std::ios::trunc);
			duplicate
				<< "peglin_settings_version=1\n"
				<< "difficulty=normal\n"
				<< "difficulty=hard\n"
				<< "sound_enabled=1\n"
				<< "peg_color_mode=standard\n";
		}
		Check(store.Load().state == SettingsLoadState::Invalid, "duplicate settings key is rejected");
		{
			std::ofstream malformed(settingsPath, std::ios::trunc);
			malformed << "peglin_settings_version=1\ndifficulty\n";
		}
		Check(store.Load().state == SettingsLoadState::Invalid, "malformed settings line is rejected");

		const std::filesystem::path blockedParent = testDirectory / "blocked-parent";
		{
			std::ofstream blocker(blockedParent, std::ios::trunc);
			blocker << "file blocks directory creation";
		}
		GameSettingsStore blockedStore(blockedParent / "settings.v1.ini");
		std::string blockedError;
		Check(!blockedStore.Save(saved, &blockedError), "settings save fails safely for an unwritable path shape");
		Check(!blockedError.empty(), "failed settings save reports an error");

		std::filesystem::remove(blockedParent, cleanupError);
		std::filesystem::remove(settingsPath, cleanupError);
		std::filesystem::remove(settingsPath.wstring() + L".tmp", cleanupError);
		std::filesystem::remove(settingsPath.parent_path(), cleanupError);
		std::filesystem::remove(testDirectory, cleanupError);
	}

	void TestStageRecordPersistence()
	{
		GameRecordBook records;
		const StageRecord empty = records.Get("stage-1", GameDifficulty::Normal);
		Check(empty.highScore == 0, "missing stage record has zero score");
		Check(empty.bestCombo == 0, "missing stage record has zero combo");
		Check(empty.clearCount == 0, "missing stage record has zero clears");
		Check(!records.ApplyResult("bad|stage", GameDifficulty::Normal, 10, 1, true), "invalid stage id is not recorded");
		Check(!records.ApplyResult("stage-1", GameDifficulty::Normal, -1, 1, false), "negative score is not recorded");
		Check(!records.ApplyResult("stage-1", GameDifficulty::Normal, 0, 0, false), "empty defeat does not create a record");

		Check(records.ApplyResult("stage-1", GameDifficulty::Normal, 500, 4, false), "first scored run creates a record");
		StageRecord normal = records.Get("stage-1", GameDifficulty::Normal);
		Check(normal.highScore == 500, "first run stores high score");
		Check(normal.bestCombo == 4, "first run stores best combo");
		Check(normal.clearCount == 0, "defeat does not increment clear count");
		Check(!records.ApplyResult("stage-1", GameDifficulty::Normal, 300, 2, false), "lower defeat does not overwrite a record");
		normal = records.Get("stage-1", GameDifficulty::Normal);
		Check(normal.highScore == 500 && normal.bestCombo == 4, "lower result preserves both records");

		Check(records.ApplyResult("stage-1", GameDifficulty::Normal, 400, 3, true), "victory updates clear count even with lower stats");
		normal = records.Get("stage-1", GameDifficulty::Normal);
		Check(normal.highScore == 500, "lower victory preserves high score");
		Check(normal.bestCombo == 4, "lower victory preserves best combo");
		Check(normal.clearCount == 1, "victory increments clear count");
		Check(records.ApplyResult("stage-1", GameDifficulty::Normal, 900, 8, true), "better victory updates all record values");
		normal = records.Get("stage-1", GameDifficulty::Normal);
		Check(normal.highScore == 900, "better victory replaces high score");
		Check(normal.bestCombo == 8, "better victory replaces best combo");
		Check(normal.clearCount == 2, "second victory increments clear count again");

		Check(records.ApplyResult("stage-1", GameDifficulty::Hard, 700, 6, true), "difficulty creates an isolated record");
		const StageRecord hard = records.Get("stage-1", GameDifficulty::Hard);
		Check(hard.highScore == 700 && hard.clearCount == 1, "hard record retains its own result");
		Check(records.Get("stage-1", GameDifficulty::Normal).highScore == 900, "normal record remains isolated from hard");
		Check(records.ApplyResult("stage-2", GameDifficulty::Normal, 1200, 10, true), "stage id creates an isolated record");
		Check(records.Get("stage-2", GameDifficulty::Normal).highScore == 1200, "second stage retains its own result");

		const std::filesystem::path testDirectory =
			std::filesystem::temp_directory_path()
			/ ("PeglinMFC_RecordStoreTests_" + std::to_string(::GetCurrentProcessId()));
		const std::filesystem::path recordPath = testDirectory / "nested" / "records.v1.ini";
		std::error_code cleanupError;
		std::filesystem::remove(recordPath, cleanupError);
		std::filesystem::remove(recordPath.wstring() + L".tmp", cleanupError);
		std::filesystem::remove(recordPath.parent_path(), cleanupError);
		std::filesystem::remove(testDirectory, cleanupError);

		GameRecordStore store(recordPath);
		Check(store.Load().state == RecordLoadState::Missing, "missing records use a distinct load state");
		std::string saveError;
		Check(store.Save(records, &saveError), "record save creates missing parent directories");
		Check(saveError.empty(), "successful record save clears its error");
		const RecordLoadResult loaded = store.Load();
		Check(loaded.state == RecordLoadState::Loaded, "valid records load from disk");
		Check(loaded.records.GetAll().size() == 3, "record load restores every stage and difficulty key");
		const StageRecord loadedNormal = loaded.records.Get("stage-1", GameDifficulty::Normal);
		Check(loadedNormal.highScore == 900, "record file preserves high score");
		Check(loadedNormal.bestCombo == 8, "record file preserves best combo");
		Check(loadedNormal.clearCount == 2, "record file preserves clear count");
		Check(loaded.records.Get("stage-1", GameDifficulty::Hard).highScore == 700, "record file preserves difficulty isolation");
		Check(loaded.records.Get("stage-2", GameDifficulty::Normal).highScore == 1200, "record file preserves stage isolation");

		GameRecordBook updated = loaded.records;
		Check(updated.ApplyResult("stage-2", GameDifficulty::Normal, 1500, 11, false), "record book accepts a later improvement");
		Check(store.Save(updated), "record save replaces an existing file");
		Check(store.Load().records.Get("stage-2", GameDifficulty::Normal).highScore == 1500, "replacement record file contains new high score");

		{
			std::ofstream incompatible(recordPath, std::ios::trunc);
			incompatible << "peglin_records_version=999\n";
		}
		Check(store.Load().state == RecordLoadState::Invalid, "unsupported record version is rejected");
		{
			std::ofstream legacy(recordPath, std::ios::trunc);
			legacy
				<< "peglin_records_version=0\n"
				<< "record=stage-1|8800|10|2\n"
				<< "record=stage-2|1200|4|0\n";
		}
		const RecordLoadResult migratedRecords = store.Load();
		Check(migratedRecords.state == RecordLoadState::Migrated, "legacy records report migration state");
		Check(migratedRecords.records.GetAll().size() == 2, "legacy records preserve every stage");
		const StageRecord migratedStage = migratedRecords.records.Get("stage-1", GameDifficulty::Normal);
		Check(migratedStage.highScore == 8800, "legacy record score migrates");
		Check(migratedStage.bestCombo == 10, "legacy record combo migrates");
		Check(migratedStage.clearCount == 2, "legacy record clears migrate");
		Check(migratedRecords.records.Get("stage-1", GameDifficulty::Hard).highScore == 0, "legacy record defaults to normal difficulty");
		Check(store.Save(migratedRecords.records), "migrated records rewrite as version one");
		Check(store.Load().state == RecordLoadState::Loaded, "rewritten records load as current version");
		{
			std::ofstream invalidDifficulty(recordPath, std::ios::trunc);
			invalidDifficulty << "peglin_records_version=1\nrecord=stage-1|nightmare|10|1|1\n";
		}
		const RecordLoadResult unknownDifficulty = store.Load();
		Check(unknownDifficulty.state == RecordLoadState::Invalid, "unknown record difficulty is rejected");
		Check(unknownDifficulty.records.GetAll().empty(), "invalid record file does not leak partial data");
		{
			std::ofstream negative(recordPath, std::ios::trunc);
			negative << "peglin_records_version=1\nrecord=stage-1|normal|-1|1|0\n";
		}
		Check(store.Load().state == RecordLoadState::Invalid, "negative record value is rejected");
		{
			std::ofstream duplicate(recordPath, std::ios::trunc);
			duplicate
				<< "peglin_records_version=1\n"
				<< "record=stage-1|normal|10|1|0\n"
				<< "record=stage-1|normal|20|2|1\n";
		}
		Check(store.Load().state == RecordLoadState::Invalid, "duplicate record key is rejected");
		{
			std::ofstream overflow(recordPath, std::ios::trunc);
			overflow << "peglin_records_version=1\nrecord=stage-1|normal|999999999999|1|0\n";
		}
		Check(store.Load().state == RecordLoadState::Invalid, "overflowing record integer is rejected");
		{
			std::ofstream malformed(recordPath, std::ios::trunc);
			malformed << "peglin_records_version=1\nnot-a-record\n";
		}
		Check(store.Load().state == RecordLoadState::Invalid, "malformed record line is rejected");

		const std::filesystem::path blockedParent = testDirectory / "blocked-parent";
		{
			std::ofstream blocker(blockedParent, std::ios::trunc);
			blocker << "file blocks directory creation";
		}
		GameRecordStore blockedStore(blockedParent / "records.v1.ini");
		std::string blockedError;
		Check(!blockedStore.Save(records, &blockedError), "record save fails safely for an unwritable path shape");
		Check(!blockedError.empty(), "failed record save reports an error");

		std::filesystem::remove(blockedParent, cleanupError);
		std::filesystem::remove(recordPath, cleanupError);
		std::filesystem::remove(recordPath.wstring() + L".tmp", cleanupError);
		std::filesystem::remove(recordPath.parent_path(), cleanupError);
		std::filesystem::remove(testDirectory, cleanupError);
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

	void TestScoreCancellationAndContinuation()
	{
		GameWorld world;
		std::vector<Vector2> pegPositions;
		auto scan = world.GetTargets()._targetBallList.GetHeadPosition();
		while (scan != nullptr && pegPositions.size() < 2)
		{
			const TargetBall& target = world.GetTargets()._targetBallList.GetNext(scan);
			if (target.type == PegType::Normal && target.position.x > 200.0f)
			{
				pegPositions.push_back(target.position);
			}
		}
		Check(pegPositions.size() == 2, "score cancellation test finds two normal pegs");

		Launch(world);
		world.GetBall().SetPosition(pegPositions[0] + Vector2{ -15.0f, 0.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		Check(world.GetScore().currentShot == 100, "cancelled shot first records pending score");
		world.ResetBallToAiming();
		Check(world.GetScore().currentShot == 0, "F5 discards pending shot score");
		Check(world.GetScore().currentCombo == 0, "F5 discards current combo");
		Check(world.GetScore().total == 0, "F5 does not commit discarded score");
		Check(world.GetScore().bestCombo == 1, "F5 preserves achieved best combo");

		Launch(world);
		world.GetBall().SetPosition(pegPositions[1] + Vector2{ -15.0f, 0.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		Check(world.GetScore().currentCombo == 1, "shot after F5 restarts combo at one");
		Check(world.GetScore().currentShot == 100, "shot after F5 starts a fresh score");
	}

	void TestBombDoesNotChainSecondaryEffects()
	{
		PegLayoutDefinition layout;
		layout.pegs = {
			{ { 500.0f, 500.0f }, PegType::Bomb },
			{ { 580.0f, 500.0f }, PegType::Bomb },
			{ { 660.0f, 500.0f }, PegType::Normal }
		};
		GameWorld world(layout);
		Launch(world);
		world.GetBall().SetPosition({ 485.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);

		Check(world.GetTargets()._targetBallList.GetCount() == 1, "bomb removes nearby bomb without chaining it");
		const auto remaining = world.GetTargets()._targetBallList.GetHeadPosition();
		const TargetBall& target = world.GetTargets()._targetBallList.GetAt(remaining);
		Check(target.type == PegType::Normal && Near(target.position.x, 660.0f), "non-chained bomb leaves second-radius peg active");
		Check(world.GetFeedback().currentShotPegHits == 2, "non-chained bomb counts two removed pegs");
		Check(world.GetScore().currentShot == 300, "non-chained bomb keeps deterministic combo score");
	}

	void TestRefreshDoesNotDuplicateActivePegs()
	{
		PegLayoutDefinition layout;
		layout.pegs = {
			{ { 300.0f, 500.0f }, PegType::Normal },
			{ { 500.0f, 500.0f }, PegType::Refresh },
			{ { 600.0f, 500.0f }, PegType::Refresh }
		};
		GameWorld world(layout);
		Launch(world);
		world.GetBall().SetPosition({ 485.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		Check(world.GetTargets()._targetBallList.GetCount() == 3, "first refresh restores itself without duplicating positions");
		Check(CountActivePegsOfType(world, PegType::Refresh) == 2,
			"first refresh relocation preserves configured refresh count");

		Vector2 nextRefreshPosition;
		auto refreshPosition = world.GetTargets()._targetBallList.GetHeadPosition();
		while (refreshPosition != nullptr)
		{
			const TargetBall& candidate =
				world.GetTargets()._targetBallList.GetNext(refreshPosition);
			if (candidate.type == PegType::Refresh)
			{
				nextRefreshPosition = candidate.position;
				break;
			}
		}
		world.GetBall().SetPosition(nextRefreshPosition + Vector2{ -15.0f, 0.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		Check(world.GetTargets()._targetBallList.GetCount() == 3, "second refresh also restores itself without duplicates");

		int normalCount = 0;
		int refreshCount = 0;
		auto position = world.GetTargets()._targetBallList.GetHeadPosition();
		while (position != nullptr)
		{
			const TargetBall& active = world.GetTargets()._targetBallList.GetNext(position);
			normalCount += active.type == PegType::Normal ? 1 : 0;
			refreshCount += active.type == PegType::Refresh ? 1 : 0;
		}
		Check(normalCount == 1 && refreshCount == 2,
			"refresh cycle preserves the configured refresh count after relocation");
	}

	void TestGameEventFeedbackStream()
	{
		PegLayoutDefinition layout;
		layout.pegs = {
			{ { 500.0f, 500.0f }, PegType::Bomb },
			{ { 580.0f, 500.0f }, PegType::Critical },
			{ { 760.0f, 500.0f }, PegType::Refresh }
		};
		GameWorld world(layout);
		Launch(world);
		world.GetBall().SetPosition({ 485.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);

		const std::vector<GameEvent> bombEvents = world.ConsumeEvents();
		Check(bombEvents.size() == 3, "bomb collision emits two peg hits and one effect event");
		const auto bombEffect = std::find_if(
			bombEvents.begin(),
			bombEvents.end(),
			[](const GameEvent& event) { return event.type == GameEventType::BombTriggered; });
		Check(bombEffect != bombEvents.end(), "bomb collision emits BombTriggered event");
		Check(bombEffect != bombEvents.end() && bombEffect->affectedPegs == 1, "bomb event reports affected neighbors");
		const auto criticalHit = std::find_if(
			bombEvents.begin(),
			bombEvents.end(),
			[](const GameEvent& event)
			{
				return event.type == GameEventType::PegHit && event.pegType == PegType::Critical;
			});
		Check(criticalHit != bombEvents.end(), "blast removal retains critical peg identity in feedback");
		Check(criticalHit != bombEvents.end() && criticalHit->scoreAwarded == 400, "critical event reports combo-multiplied score");
		Check(world.ConsumeEvents().empty(), "consuming feedback drains the event stream");

		world.GetBall().SetPosition({ 745.0f, 500.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		const std::vector<GameEvent> refreshEvents = world.ConsumeEvents();
		const auto refreshEffect = std::find_if(
			refreshEvents.begin(),
			refreshEvents.end(),
			[](const GameEvent& event) { return event.type == GameEventType::RefreshTriggered; });
		Check(refreshEffect != refreshEvents.end(), "refresh collision emits RefreshTriggered event");
		Check(refreshEffect != refreshEvents.end() && refreshEffect->affectedPegs == 3, "refresh event reports removed pegs plus its replacement");

		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		const std::vector<GameEvent> turnEvents = world.ConsumeEvents();
		const auto turnEvent = std::find_if(
			turnEvents.begin(),
			turnEvents.end(),
			[](const GameEvent& event) { return event.type == GameEventType::TurnResolved; });
		Check(turnEvent != turnEvents.end(), "resolved shot emits TurnResolved event");
		Check(turnEvent != turnEvents.end() && turnEvent->scoreAwarded == world.GetScore().lastTurn, "turn event reports committed score");

		world.ResetGame();
		Check(world.ConsumeEvents().empty(), "new game clears pending feedback events");
	}

	void TestAimPreviewMatchesShotInput()
	{
		GameWorld world;
		const Vector2 start = world.GetBall().GetPosition();
		Check(world.BeginAim(start), "aim preview begins from the ball");
		Check(!world.GetAimPreview().visible, "zero-length aim keeps preview hidden");

		world.UpdateAim(start + Vector2{ 220.0f, 0.0f });
		const AimPreview medium = world.GetAimPreview();
		Check(medium.visible, "non-zero drag shows aim preview");
		Check(Near(medium.launchDirection.x, -1.0f) && Near(medium.launchDirection.y, 0.0f), "preview points opposite the drag direction");
		Check(Near(medium.normalizedStrength, 0.5f), "220-pixel drag maps to fifty percent strength");
		Check(medium.points.front().x < start.x, "first preview point follows launch direction");
		float previewLength = 0.0f;
		Vector2 previousPoint = start;
		for (const Vector2 point : medium.points)
		{
			previewLength += (point - previousPoint).Length();
			previousPoint = point;
		}
		Check(
			std::fabs(previewLength - AimPreview::GuideLengthPixels) < 0.25f,
			"aim guide stays near one and a half centimeters at 96 DPI");

		bool allPointsInsideWalls = true;
		for (const Vector2 point : medium.points)
		{
			allPointsInsideWalls = allPointsInsideWalls
				&& point.x >= GameLayout::BallLeftBoundary
				&& point.x <= GameLayout::BallRightBoundary
				&& point.y >= GameLayout::BallTopBoundary;
		}
		Check(allPointsInsideWalls, "preview wall reflections stay inside physical boundaries");

		PegLayoutDefinition collisionLayout;
		collisionLayout.pegs.push_back({ start + Vector2{ 40.0f, 0.0f }, PegType::Normal });
		GameWorld collisionWorld(collisionLayout);
		Check(collisionWorld.BeginAim(start), "collision preview begins from the ball");
		collisionWorld.UpdateAim(start - Vector2{ 220.0f, 0.0f });
		const AimPreview collision = collisionWorld.GetAimPreview();
		Check(collision.PredictsPegCollision(), "aim guide predicts an intersecting peg");
		Check(
			collision.firstPegCollisionPoint < collision.points.size() - 1,
			"predicted peg collision leaves room for a reflected direction");
		Check(
			collision.points.back().x
				< collision.points[collision.firstPegCollisionPoint].x,
			"aim guide shows the post-peg reflection direction");

		world.UpdateAim(start + Vector2{ 400.0f, 0.0f });
		Check(Near(world.GetAimPreview().normalizedStrength, 1.0f), "maximum drag clamps preview strength to one");
		Check(world.ReleaseShot(start + Vector2{ 400.0f, 0.0f }), "previewed drag launches the ball");
		Check(Near(world.GetBall().GetVelocity().x, -1.0f), "launched velocity matches preview direction");
		Check(!world.GetAimPreview().visible, "preview hides after launch");

		world.ResetBallToAiming();
		Check(!world.GetAimPreview().visible, "F5 leaves no stale aim preview");
	}

	void TestStageValidationMatrix()
	{
		StageDefinition invalid = CreateDefaultStageDefinition();
		invalid.pegLayout.pegs.resize(257, invalid.pegLayout.pegs.front());
		Check(ValidateStageDefinition(invalid).error == StageLoadError::TooManyPegs, "stage peg limit is enforced before duplicate scan");

		invalid = CreateDefaultStageDefinition();
		invalid.pegLayout.pegs[0].type = static_cast<PegType>(999);
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidPegType, "unknown peg enum value is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.rules.playerHealth = 0.0f;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidPlayerHealth, "non-positive player health is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.rules.playerDamage = 0.0f;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidPlayerDamage, "non-positive player damage is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.rules.enemyStep = 0.0f;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemyStep, "non-positive enemy step is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.enemies = {
			{ "effect-target", "Effect Target", EnemyVisualKind::CrystalToad, 10.0f, 0.0f }
		};
		Check(
			ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemyDamageTakenMultiplier,
			"out-of-range enemy damage effect is rejected");

		invalid = CreateDefaultStageDefinition();
		invalid.enemies = {
			{ "range-target", "Range Target", EnemyVisualKind::CrystalToad, 10.0f, 1.0f, 0 }
		};
		Check(
			ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemyAttackRange,
			"non-positive enemy attack range is rejected");

		const StageLoadResult first = LoadStageDefinition("stage-2");
		const StageLoadResult repeated = LoadStageDefinition("stage-2");
		bool deterministic = first.IsSuccess()
			&& repeated.IsSuccess()
			&& first.stage->pegLayout.pegs.size() == repeated.stage->pegLayout.pegs.size();
		for (std::size_t index = 0; deterministic && index < first.stage->pegLayout.pegs.size(); ++index)
		{
			deterministic = first.stage->pegLayout.pegs[index].type == repeated.stage->pegLayout.pegs[index].type
				&& Near(first.stage->pegLayout.pegs[index].position.x, repeated.stage->pegLayout.pegs[index].position.x)
				&& Near(first.stage->pegLayout.pegs[index].position.y, repeated.stage->pegLayout.pegs[index].position.y);
		}
		Check(deterministic, "repeated stage-2 loads are deterministic");
	}

	void TestBossEnemyActionPattern()
	{
		const StageDefinition boss = CreateBossStageDefinition();
		Check(ValidateStageDefinition(boss).IsValid(), "boss stage definition validates");
		Check(boss.isBoss, "boss stage keeps its boss marker");
		Check(boss.enemyPattern.size() == 4, "boss pattern has four telegraphed actions");
		Check(boss.enemyPattern[0].type == EnemyActionType::Strike, "boss opens its in-range pattern with a strike");
		Check(boss.enemyPattern[1].type == EnemyActionType::Fortify, "boss follows with fortify");
		Check(boss.enemyPattern[2].type == EnemyActionType::Strike, "boss third action is a strike");
		Check(boss.enemyPattern[3].type == EnemyActionType::Strike, "boss fourth action is a heavy strike");
		Check(boss.enemies[0].attackRangeCells == 2, "boss has an explicit two-cell attack range");

		const StageDefinition easy = ApplyDifficulty(boss, GameDifficulty::Easy);
		const StageDefinition hard = ApplyDifficulty(boss, GameDifficulty::Hard);
		Check(Near(easy.enemyPattern[0].magnitude, 13.5f), "easy difficulty scales the boss opening strike");
		Check(Near(easy.enemyPattern[2].magnitude, 13.5f), "easy difficulty scales boss strike damage");
		Check(Near(easy.enemyPattern[3].magnitude, 18.0f), "easy difficulty scales boss heavy strike damage");
		Check(Near(hard.enemyPattern[0].magnitude, 22.5f), "hard difficulty scales the boss opening strike");
		Check(Near(hard.enemyPattern[2].magnitude, 22.5f), "hard difficulty scales boss strike damage");
		Check(Near(hard.enemyPattern[3].magnitude, 30.0f), "hard difficulty scales boss heavy strike damage");
		Check(Near(hard.enemyPattern[1].magnitude, 4.0f), "difficulty does not scale boss shield");

		StageDefinition invalid = boss;
		invalid.enemyPattern.clear();
		Check(ValidateStageDefinition(invalid).error == StageLoadError::MissingBossPattern, "boss without a pattern is rejected");
		invalid = boss;
		invalid.enemyPattern[0].type = static_cast<EnemyActionType>(999);
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemyAction, "unknown enemy action is rejected");
		invalid = boss;
		invalid.enemyPattern[0].type = EnemyActionType::Advance;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemyAction, "boss pattern cannot resume movement inside attack range");
		invalid = boss;
		invalid.enemyPattern[1].magnitude = 0.0f;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemyActionMagnitude, "non-positive enemy action is rejected");

		auto ResolveEmptyTurn = [](GameWorld& world)
		{
			Launch(world);
			world.GetBall().SetPosition({ 500.0f, 801.0f });
			world.Update(0.0f);
			world.Update(0.0f);
		};
		auto HasEvent = [](const std::vector<GameEvent>& events, GameEventType type)
		{
			return std::any_of(events.begin(), events.end(), [type](const GameEvent& event)
			{
				return event.type == type;
			});
		};

		GameWorld world(boss);
		Check(world.GetEnemies()[0].distanceToPlayerCells == 4, "boss starts four cells from the player");
		Check(world.GetNextEnemyAction().type == EnemyActionType::Advance, "out-of-range boss previews one-cell movement");
		const float initialEnemyX = world.GetEnemy().GetX();
		ResolveEmptyTurn(world);
		Check(Near(world.GetEnemy().GetX(), initialEnemyX - 48.0f), "advance action moves the boss by its magnitude");
		Check(world.GetEnemies()[0].distanceToPlayerCells == 3, "first boss move reduces distance by one cell");
		Check(world.GetNextEnemyAction().type == EnemyActionType::Advance, "boss keeps moving while outside range");
		Check(HasEvent(world.ConsumeEvents(), GameEventType::EnemyAdvanced), "advance action emits feedback event");

		ResolveEmptyTurn(world);
		Check(world.GetEnemies()[0].distanceToPlayerCells == 2, "second boss move reaches its attack range");
		Check(world.GetNextEnemyAction().type == EnemyActionType::Strike, "boss stops moving and previews a strike in range");
		Check(Near(world.GetPlayer().GetHp(), 110.0f), "entering range does not attack in the same turn");

		ResolveEmptyTurn(world);
		Check(Near(world.GetPlayer().GetHp(), 92.0f), "boss opening strike applies after range entry");
		Check(world.GetNextEnemyAction().type == EnemyActionType::Fortify, "fortify follows the opening strike");

		ResolveEmptyTurn(world);
		Check(Near(world.GetEnemyShield(), 4.0f), "fortify action grants boss shield");
		Check(world.GetNextEnemyAction().type == EnemyActionType::Strike, "strike is previewed after fortify");
		Check(HasEvent(world.ConsumeEvents(), GameEventType::EnemyFortified), "fortify action emits feedback event");

		const float currentOrbDamage = world.GetProgressionModifiers().pegDamageMultiplier;
		Launch(world);
		auto scan = world.GetTargets()._targetBallList.GetHeadPosition();
		Vector2 normalPeg;
		while (scan != nullptr)
		{
			const TargetBall& candidate = world.GetTargets()._targetBallList.GetNext(scan);
			if (candidate.type == PegType::Normal)
			{
				normalPeg = candidate.position;
				break;
			}
		}
		world.GetBall().SetPosition(normalPeg + Vector2{ -15.0f, 0.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		Check(Near(world.GetEnemyShield(), 4.0f - currentOrbDamage), "boss shield absorbs current orb damage first");
		Check(Near(world.GetEnemy().GetHp(), 60.0f), "absorbed peg damage does not reduce boss health");
		Check(Near(world.GetPlayer().GetHp(), 74.0f), "boss strike applies its telegraphed damage");
		Check(Near(world.GetFeedback().lastEnemyDamage, 0.0f), "absorbed damage is excluded from resolved enemy damage");

		ResolveEmptyTurn(world);
		Check(Near(world.GetPlayer().GetHp(), 50.0f), "boss heavy strike applies its telegraphed damage");
		Check(world.GetNextEnemyAction().type == EnemyActionType::Strike, "boss attack pattern loops without resuming movement");
		world.ResetGame();
		Check(Near(world.GetEnemyShield(), 0.0f), "retry clears boss shield");
		Check(world.GetEnemy().GetCount() == 0, "retry resets boss action index");
		Check(world.GetEnemies()[0].distanceToPlayerCells == 4, "retry restores boss distance");
		Check(world.GetNextEnemyAction().type == EnemyActionType::Advance, "retry restores movement outside range");
	}

	void TestExternalContentCatalog()
	{
		const std::filesystem::path repositoryRoot =
			std::filesystem::path(__FILE__).parent_path().parent_path();
		const ContentLoadResult shipped = LoadContentCatalog(
			repositoryRoot / "FinalProject_Peglin" / "content" / "stages.v1.ini");
		Check(shipped.UsedExternalContent(), "shipped versioned stage catalog validates");
		Check(shipped.stages.size() == 8, "shipped external catalog exposes eight route stages");
		const StageDefinition* shippedForest = FindContentStage(shipped.stages, "stage-1");
		const StageDefinition* shippedCavern = FindContentStage(shipped.stages, "stage-2");
		const StageDefinition* shippedThornwood = FindContentStage(shipped.stages, "stage-4");
		const StageDefinition* shippedFungal = FindContentStage(shipped.stages, "stage-5");
		Check(shippedForest != nullptr && shippedForest->enemies.size() == 3, "shipped forest exposes a three-monster roster");
		Check(shippedCavern != nullptr && shippedCavern->enemies.size() == 3, "shipped cavern exposes a three-monster roster");
		Check(
			shippedForest != nullptr
			&& shippedForest->enemies[0].attackRangeCells == 1
			&& shippedForest->enemies[1].attackRangeCells == 3
			&& shippedForest->enemies[2].attackRangeCells == 2,
			"shipped forest preserves distinct per-monster attack ranges");
		Check(
			shippedForest != nullptr && Near(shippedForest->rules.enemyStep, 32.0f),
			"shipped group encounters use a non-overlapping visual cell step");
		Check(
			shippedThornwood != nullptr
			&& shippedThornwood->enemies[0].visual == EnemyVisualKind::ThornbackWolf,
			"shipped catalog parses the thornback wolf visual");
		Check(
			shippedFungal != nullptr
			&& shippedFungal->enemies[1].visual == EnemyVisualKind::AzureWisp,
			"shipped catalog parses the azure wisp visual");
		const StageDefinition* shippedBoss = FindContentStage(shipped.stages, "stage-3");
		Check(shippedBoss != nullptr && shippedBoss->enemies.size() == 1, "shipped boss exposes one named boss");
		Check(shippedBoss != nullptr && shippedBoss->enemyPattern.size() == 4, "shipped external catalog includes the boss pattern");
		std::vector<RunStageEntry> shippedEntries;
		for (const StageDefinition& stage : shipped.stages)
		{
			shippedEntries.push_back({ stage.id, stage.isBoss });
		}
		const RunStageLayers shippedRoute = BuildBranchingStageLayers(shippedEntries);
		Check(shippedRoute.size() == 6, "shipped content supports a shop-inclusive six-layer boss route");

		const std::filesystem::path testDirectory =
			std::filesystem::temp_directory_path()
			/ ("PeglinMFC_ContentCatalogTests_" + std::to_string(::GetCurrentProcessId()));
		const std::filesystem::path contentPath = testDirectory / "stages.v1.ini";
		std::error_code cleanupError;
		std::filesystem::remove_all(testDirectory, cleanupError);
		std::filesystem::create_directories(testDirectory, cleanupError);
		Check(!cleanupError, "content test directory is available");

		const std::string validContent =
			"version=1\n"
			"[stage]\n"
			"id=external-stage\n"
			"name=External Trial\n"
			"layout=2,2,300,400,80,0,7\n"
			"peg_type=1,Critical\n"
			"peg_motion=2,Vertical,20,1.5,0\n"
			"player_health=90\n"
			"enemy_health=45\n"
			"player_damage=12\n"
			"enemy_steps=3\n"
			"enemy_step=40\n"
			"restitution=0.75\n"
			"boss=true\n"
			"action=Fortify,3\n"
			"action=Strike,12\n"
			"[/stage]\n";
		auto WriteContent = [&contentPath](std::string_view content)
		{
			std::ofstream output(contentPath, std::ios::binary | std::ios::trunc);
			output.write(content.data(), static_cast<std::streamsize>(content.size()));
		};

		WriteContent(validContent);
		const ContentLoadResult valid = LoadContentCatalog(contentPath);
		Check(valid.UsedExternalContent(), "valid external content replaces built-in catalog");
		Check(valid.error == ContentLoadError::None, "valid external content has no load error");
		Check(valid.stages.size() == 1, "external catalog exposes only fully validated stages");
		const StageDefinition* external = FindContentStage(valid.stages, "external-stage");
		Check(external != nullptr, "external stage can be found by stable id");
		Check(external != nullptr && external->pegLayout.pegs.size() == 4, "external grid definition creates its peg board");
		Check(external != nullptr && external->pegLayout.pegs[1].type == PegType::Critical, "external peg override is applied");
		Check(
			external != nullptr
			&& external->pegLayout.pegs[2].motion.kind == PegMotionKind::Vertical
			&& Near(external->pegLayout.pegs[2].motion.amplitude, 20.0f),
			"external peg motion preserves its axis and amplitude");
		Check(external != nullptr && external->enemyPattern.size() == 2, "external enemy actions preserve order");
		GameWorld externalWorld;
		Check(external != nullptr && externalWorld.LoadStage(*external, GameDifficulty::Hard), "game world accepts validated external stage data");
		Check(Near(externalWorld.GetEnemy().GetHp(), 67.5f), "difficulty applies after external stage validation");
		Check(Near(externalWorld.GetNextEnemyAction().magnitude, 3.0f), "non-strike external action keeps its magnitude");

		std::string rosterContent = validContent;
		rosterContent.insert(
			rosterContent.find("player_damage="),
			"enemy=crystal-toad,Crystal Toad,CrystalToad,15,1\n"
			"enemy=ember-bat,Ember Bat,EmberBat,12,3\n"
			"enemy=moss-shaman,Moss Shaman,MossShaman,18,2\n");
		WriteContent(rosterContent);
		const ContentLoadResult roster = LoadContentCatalog(contentPath);
		Check(roster.UsedExternalContent(), "external content accepts repeated enemy roster entries");
		const StageDefinition* rosterStage = FindContentStage(roster.stages, "external-stage");
		Check(rosterStage != nullptr && rosterStage->enemies.size() == 3, "external roster preserves all enemy entries");
		Check(
			rosterStage != nullptr && rosterStage->enemies[1].visual == EnemyVisualKind::EmberBat,
			"external roster parses enemy visual kinds");
		Check(
			rosterStage != nullptr && rosterStage->enemies[1].attackRangeCells == 3,
			"external roster parses per-monster attack ranges");
		GameWorld rosterWorld;
		Check(rosterStage != nullptr && rosterWorld.LoadStage(*rosterStage, GameDifficulty::Hard), "world loads external enemy roster");
		Check(Near(rosterWorld.GetEnemy().GetHp(), 22.5f), "difficulty scales each roster enemy health");

		const ContentLoadResult missing = LoadContentCatalog(testDirectory / "missing.ini");
		Check(missing.state == ContentLoadState::BuiltInFallback, "missing content uses built-in fallback");
		Check(missing.error == ContentLoadError::MissingFile, "missing content reports its cause");
		Check(missing.stages.size() == 8, "missing content recovers the complete built-in route catalog");
		Check(FindContentStage(missing.stages, "stage-3") != nullptr, "fallback includes the boss stage");

		WriteContent("version=99\n");
		const ContentLoadResult unsupported = LoadContentCatalog(contentPath);
		Check(unsupported.error == ContentLoadError::UnsupportedVersion, "unsupported content version is rejected");
		Check(unsupported.stages.size() == 8, "unsupported version exposes no partial external data");

		WriteContent(validContent + validContent.substr(validContent.find("[stage]")));
		const ContentLoadResult duplicateId = LoadContentCatalog(contentPath);
		Check(duplicateId.error == ContentLoadError::DuplicateStageId, "duplicate external stage id is rejected");

		std::string duplicateKey = validContent;
		duplicateKey.insert(duplicateKey.find("enemy_health="), "player_health=80\n");
		WriteContent(duplicateKey);
		Check(LoadContentCatalog(contentPath).error == ContentLoadError::DuplicateKey, "duplicate scalar content key is rejected");

		std::string unknownKey = validContent;
		unknownKey.insert(unknownKey.find("[/stage]"), "mystery=1\n");
		WriteContent(unknownKey);
		Check(LoadContentCatalog(contentPath).error == ContentLoadError::UnknownKey, "unknown content key is rejected");

		std::string badPegIndex = validContent;
		badPegIndex.replace(badPegIndex.find("peg_type=1"), 10, "peg_type=99");
		WriteContent(badPegIndex);
		Check(LoadContentCatalog(contentPath).error == ContentLoadError::InvalidValue, "out-of-range peg override is rejected");

		std::string duplicateMotion = validContent;
		duplicateMotion.insert(duplicateMotion.find("player_health="),
			"peg_motion=2,Horizontal,10,1,0\n");
		WriteContent(duplicateMotion);
		Check(LoadContentCatalog(contentPath).error == ContentLoadError::DuplicateKey,
			"duplicate external peg motion is rejected");

		std::string partial = validContent;
		partial.erase(partial.find("enemy_health="), std::string("enemy_health=45\n").size());
		WriteContent(partial);
		Check(LoadContentCatalog(contentPath).error == ContentLoadError::MissingField, "partial stage definition is rejected as a whole");

		WriteContent(validContent.substr(0, validContent.find("[/stage]")));
		Check(LoadContentCatalog(contentPath).error == ContentLoadError::UnexpectedSection, "truncated stage section is rejected");

		std::string invalidUtf8 = validContent;
		invalidUtf8.insert(invalidUtf8.find("External"), 1, static_cast<char>(0xFF));
		WriteContent(invalidUtf8);
		Check(LoadContentCatalog(contentPath).error == ContentLoadError::InvalidEncoding, "invalid UTF-8 content is rejected");

		WriteContent(std::string(256 * 1024 + 1, 'x'));
		Check(LoadContentCatalog(contentPath).error == ContentLoadError::FileTooLarge, "oversized content file is rejected before parsing");

		const ContentLoadResult directoryRead = LoadContentCatalog(testDirectory);
		Check(directoryRead.error == ContentLoadError::IoFailure, "non-file content path fails safely");
		std::filesystem::remove_all(testDirectory, cleanupError);
	}

	void TestExternalGameplayCatalog()
	{
		const std::filesystem::path repositoryRoot =
			std::filesystem::path(__FILE__).parent_path().parent_path();
		const ContentLoadResult stageCatalog = LoadContentCatalog(
			repositoryRoot / "FinalProject_Peglin" / "content" / "stages.v1.ini");
		const std::filesystem::path shippedPath = repositoryRoot
			/ "FinalProject_Peglin" / "content" / "gameplay.v1.ini";
		const GameplayCatalogLoadResult shipped = LoadGameplayCatalog(
			shippedPath,
			stageCatalog.stages);
		Check(shipped.UsedExternalContent(), "shipped gameplay definition catalog validates");
		Check(shipped.catalog.progression.orbs.size() == 3, "external gameplay catalog provides three orbs");
		Check(shipped.catalog.progression.relics.size() == 3, "external gameplay catalog provides three relics");
		Check(shipped.catalog.enemies.size() == 9, "external gameplay catalog covers every shipped enemy id");

		std::vector<StageDefinition> activatedStages = stageCatalog.stages;
		Check(ActivateGameplayCatalog(shipped, activatedStages), "validated gameplay catalog activates atomically");
		const OrbDefinition* iron = FindOrbDefinition("iron-orb");
		Check(iron != nullptr && Near(iron->pegDamageMultiplier, 1.5f), "external orb effect resolves by stable id");
		Check(iron != nullptr && iron->imageKey == "orb-iron-v1", "external orb catalog retains its image key");
		const RelicDefinition* bark = FindRelicDefinition("bark-guard");
		Check(bark != nullptr && Near(bark->incomingDamageMultiplier, 0.85f), "external relic effect resolves by stable id");
		Check(bark != nullptr && bark->imageKey == "relic-bark-guard-v1", "external relic catalog retains its image key");
		const StageDefinition* forest = FindContentStage(activatedStages, "stage-1");
		const StageDefinition* boss = FindContentStage(activatedStages, "stage-3");
		Check(forest != nullptr && Near(forest->enemies[0].damageTakenMultiplier, 0.9f), "enemy effect is applied through the stage enemy id");
		Check(boss != nullptr && Near(boss->enemies[0].damageTakenMultiplier, 0.81f), "included enemy effects compose deterministically");

		if (forest != nullptr)
		{
			GameWorld effectWorld(*forest);
			Launch(effectWorld);
			auto scan = effectWorld.GetTargets()._targetBallList.GetHeadPosition();
			Vector2 normalPeg;
			while (scan != nullptr)
			{
				const TargetBall& candidate = effectWorld.GetTargets()._targetBallList.GetNext(scan);
				if (candidate.type == PegType::Normal)
				{
					normalPeg = candidate.position;
					break;
				}
			}
			effectWorld.GetBall().SetPosition(normalPeg + Vector2{ -15.0f, 0.0f });
			effectWorld.GetBall().SetVelocity({ 2.0f, 0.0f });
			effectWorld.Update(0.0f);
			effectWorld.GetBall().SetPosition({ 500.0f, 801.0f });
			effectWorld.Update(0.0f);
			effectWorld.Update(0.0f);
			Check(Near(effectWorld.GetEnemy().GetHp(), 7.1f), "enemy damage-taken effect changes resolved combat damage");
		}

		std::ifstream shippedInput(shippedPath, std::ios::binary);
		const std::string validContent{
			std::istreambuf_iterator<char>(shippedInput),
			std::istreambuf_iterator<char>() };
		const std::filesystem::path testDirectory =
			std::filesystem::temp_directory_path()
			/ ("PeglinMFC_GameplayCatalogTests_" + std::to_string(::GetCurrentProcessId()));
		const std::filesystem::path contentPath = testDirectory / "gameplay.v1.ini";
		std::error_code cleanupError;
		std::filesystem::remove_all(testDirectory, cleanupError);
		std::filesystem::create_directories(testDirectory, cleanupError);
		Check(!cleanupError, "gameplay catalog test directory is available");
		auto WriteContent = [&contentPath](std::string_view content)
		{
			std::ofstream output(contentPath, std::ios::binary | std::ios::trunc);
			output.write(content.data(), static_cast<std::streamsize>(content.size()));
		};

		std::string duplicateId = validContent;
		const std::size_t firstEffectEnd = duplicateId.find("[/effect]") + std::string("[/effect]\n").size();
		duplicateId.insert(firstEffectEnd, validContent.substr(validContent.find("[effect]"), firstEffectEnd - validContent.find("[effect]")));
		WriteContent(duplicateId);
		Check(LoadGameplayCatalog(contentPath, stageCatalog.stages).error == GameplayCatalogLoadError::DuplicateId,
			"duplicate external effect id is rejected");

		std::string unknownEffect = validContent;
		unknownEffect.replace(
			unknownEffect.find("effect=orb-traveler-damage"),
			std::string("effect=orb-traveler-damage").size(),
			"effect=unknown-effect");
		WriteContent(unknownEffect);
		Check(LoadGameplayCatalog(contentPath, stageCatalog.stages).error == GameplayCatalogLoadError::UnknownEffectReference,
			"unknown external effect reference is rejected");

		std::string circular = validContent;
		const std::string firstEffect = "id=orb-traveler-damage\nkind=PegDamageMultiplier\nvalue=1.0\n";
		const std::string secondEffect = "id=orb-traveler-score\nkind=ScoreMultiplier\nvalue=1.0\n";
		circular.insert(circular.find(firstEffect) + firstEffect.size(), "include=orb-traveler-score\n");
		circular.insert(circular.find(secondEffect) + secondEffect.size(), "include=orb-traveler-damage\n");
		WriteContent(circular);
		Check(LoadGameplayCatalog(contentPath, stageCatalog.stages).error == GameplayCatalogLoadError::CircularEffectReference,
			"circular external effect references are rejected");

		std::vector<StageDefinition> unknownEnemyStages = stageCatalog.stages;
		unknownEnemyStages[0].enemies[0].id = "unknown-enemy";
		WriteContent(validContent);
		Check(LoadGameplayCatalog(contentPath, unknownEnemyStages).error == GameplayCatalogLoadError::UnknownEnemyReference,
			"unknown stage enemy reference rejects the external gameplay catalog");

		const GameplayCatalogLoadResult missing = LoadGameplayCatalog(testDirectory / "missing.ini", stageCatalog.stages);
		Check(missing.state == GameplayCatalogLoadState::BuiltInFallback, "missing gameplay catalog uses built-in fallback");
		Check(missing.error == GameplayCatalogLoadError::MissingFile, "missing gameplay catalog reports its cause");
		std::vector<StageDefinition> fallbackStages = stageCatalog.stages;
		Check(ActivateGameplayCatalog(missing, fallbackStages), "built-in progression fallback activates safely");
		Check(Near(FindOrbDefinition("iron-orb")->pegDamageMultiplier, 1.5f), "built-in fallback preserves the default orb catalog");

		ResetProgressionCatalog();
		std::filesystem::remove_all(testDirectory, cleanupError);
	}

	void TestLocalizationCatalog()
	{
		const std::filesystem::path repositoryRoot =
			std::filesystem::path(__FILE__).parent_path().parent_path();
		const std::filesystem::path koreanPath = repositoryRoot
			/ "FinalProject_Peglin" / "content" / "strings.ko-KR.v1.ini";
		const std::filesystem::path englishPath = repositoryRoot
			/ "FinalProject_Peglin" / "content" / "strings.en-US.v1.ini";
		const LocalizationLoadResult korean = LoadLocalizationCatalog(koreanPath, UiLanguage::Korean);
		const LocalizationLoadResult english = LoadLocalizationCatalog(englishPath, UiLanguage::English);
		Check(korean.UsedExternalContent(), "shipped Korean UTF-8 string catalog validates");
		Check(english.UsedExternalContent(), "shipped English UTF-8 string catalog validates");
		Check(korean.fallbackKeyCount == 0, "shipped Korean catalog defines every required key");
		Check(english.fallbackKeyCount == 0, "shipped English catalog defines every required key");
		Check(korean.catalog.Get("screen.options") == "옵션", "Korean catalog preserves non-ASCII text");
		Check(english.catalog.Get("screen.options") == "OPTIONS", "English catalog selects the matching locale");

		const std::filesystem::path testDirectory =
			std::filesystem::temp_directory_path()
			/ ("PeglinMFC_LocalizationTests_" + std::to_string(::GetCurrentProcessId()));
		const std::filesystem::path localizationPath = testDirectory / "strings.test.ini";
		std::error_code cleanupError;
		std::filesystem::remove_all(testDirectory, cleanupError);
		std::filesystem::create_directories(testDirectory, cleanupError);
		Check(!cleanupError, "localization test directory is available");
		auto WriteContent = [&localizationPath](std::string_view content)
		{
			std::ofstream output(localizationPath, std::ios::binary | std::ios::trunc);
			output.write(content.data(), static_cast<std::streamsize>(content.size()));
		};

		const std::string partial =
			"version=1\n"
			"locale=ko-KR\n"
			"screen.options=테스트 옵션\n";
		WriteContent(partial);
		const LocalizationLoadResult merged = LoadLocalizationCatalog(localizationPath, UiLanguage::Korean);
		Check(merged.state == LocalizationLoadState::ExternalWithFallback, "partial localization merges with built-in fallback");
		Check(merged.catalog.Get("screen.options") == "테스트 옵션", "external localization overrides a known key");
		Check(merged.catalog.Get("hint.back") == "[B] 또는 ESC로 돌아가기", "missing localization key recovers built-in text");
		Check(merged.fallbackKeyCount + 1 == merged.catalog.Size(), "missing key count reports every recovered string");

		WriteContent("version=1\nlocale=en-US\nscreen.options=OPTIONS\n");
		Check(
			LoadLocalizationCatalog(localizationPath, UiLanguage::Korean).error == LocalizationLoadError::LocaleMismatch,
			"mismatched locale file is rejected");

		WriteContent(partial + "screen.options=중복\n");
		Check(
			LoadLocalizationCatalog(localizationPath, UiLanguage::Korean).error == LocalizationLoadError::DuplicateKey,
			"duplicate localization key is rejected");

		WriteContent(partial + "unknown.key=value\n");
		Check(
			LoadLocalizationCatalog(localizationPath, UiLanguage::Korean).error == LocalizationLoadError::UnknownKey,
			"unknown localization key is rejected");

		WriteContent("version=1\nlocale=ko-KR\nscreen.options=bad\\escape\n");
		Check(
			LoadLocalizationCatalog(localizationPath, UiLanguage::Korean).error == LocalizationLoadError::InvalidValue,
			"invalid localization escape sequence is rejected");

		std::string invalidUtf8 = partial;
		invalidUtf8.push_back(static_cast<char>(0xFF));
		WriteContent(invalidUtf8);
		Check(
			LoadLocalizationCatalog(localizationPath, UiLanguage::Korean).error == LocalizationLoadError::InvalidEncoding,
			"invalid localization UTF-8 is rejected");

		const LocalizationLoadResult missing = LoadLocalizationCatalog(
			testDirectory / "missing.ini",
			UiLanguage::English);
		Check(missing.state == LocalizationLoadState::BuiltInFallback, "missing localization file uses built-in fallback");
		Check(missing.catalog.Get("screen.options") == "OPTIONS", "fallback language matches the requested locale");
		std::filesystem::remove_all(testDirectory, cleanupError);
	}

	void TestMultipleEnemyEncounter()
	{
		StageDefinition stage = CreateDefaultStageDefinition();
		stage.id = "enemy-roster-test";
		stage.enemies = {
			{ "crystal-toad", "Crystal Toad", EnemyVisualKind::CrystalToad, 8.0f },
			{ "ember-bat", "Ember Bat", EnemyVisualKind::EmberBat, 5.0f },
			{ "moss-shaman", "Moss Shaman", EnemyVisualKind::MossShaman, 7.0f }
		};
		Check(ValidateStageDefinition(stage).IsValid(), "three-monster encounter validates");

		GameWorld world(stage);
		Check(world.GetEnemies().size() == 3, "world creates every configured monster");
		Check(world.GetLivingEnemyCount() == 3, "all configured monsters begin alive");
		Check(world.GetActiveEnemyIndex() == 0, "first monster begins as the active target");
		Check(world.GetActiveEnemyDefinition().id == "crystal-toad", "active target keeps its stable monster id");
		Check(Near(world.GetEnemy().GetHp(), 8.0f), "active target uses its own health");
		Check(Near(world.GetEnemies()[0].HealthFraction(), 1.0f), "monster health progress begins full");
		world.GetEnemy().SetHp(2.0f);
		Check(Near(world.GetEnemies()[0].HealthFraction(), 0.25f), "monster health progress tracks current health");
		world.GetEnemy().SetHp(20.0f);
		Check(Near(world.GetEnemies()[0].HealthFraction(), 1.0f), "monster health progress clamps above maximum");
		world.GetEnemy().SetHp(8.0f);
		Check(Near(world.GetEnemies()[1].actor.GetX(), GameLayout::EnemyGroupStartX + GameLayout::EnemyGroupSpacing), "monster roster uses distinct stage positions");

		auto DefeatActive = [&world]()
		{
			world.GetEnemy().SetHp(0.0f);
			Launch(world);
			world.GetBall().SetPosition({ 500.0f, 801.0f });
			world.Update(0.0f);
			return world.Update(0.0f);
		};

		Check(DefeatActive() == GameUpdateResult::None, "first monster defeat keeps encounter running");
		Check(world.GetState() == GameState::Aiming, "first defeat returns to aiming for the next monster");
		Check(world.GetActiveEnemyIndex() == 1, "first defeat selects the second monster");
		Check(world.GetLivingEnemyCount() == 2, "first defeat reduces living monster count");
		const std::vector<GameEvent> firstDefeatEvents = world.ConsumeEvents();
		Check(
			std::any_of(
				firstDefeatEvents.begin(),
				firstDefeatEvents.end(),
				[](const GameEvent& event)
				{
					return event.type == GameEventType::EnemyDefeated && event.affectedPegs == 2;
				}),
			"monster defeat emits remaining-roster feedback");

		Check(DefeatActive() == GameUpdateResult::None, "second monster defeat keeps final target active");
		Check(world.GetActiveEnemyIndex() == 2, "second defeat selects the final monster");
		Check(world.GetActiveEnemyDefinition().visual == EnemyVisualKind::MossShaman, "final target preserves its distinct visual kind");
		Check(DefeatActive() == GameUpdateResult::Victory, "last monster defeat completes the stage");
		Check(world.GetLivingEnemyCount() == 0, "victory leaves no living monsters");
		Check(world.Update(0.0f) == GameUpdateResult::None, "multi-monster victory result remains single-shot");

		world.ResetGame();
		Check(world.GetActiveEnemyIndex() == 0 && world.GetLivingEnemyCount() == 3, "retry restores the complete monster roster");
		Check(Near(world.GetEnemy().GetHp(), 8.0f), "retry restores first monster health");

		StageDefinition invalid = stage;
		invalid.enemies[1].id = invalid.enemies[0].id;
		Check(ValidateStageDefinition(invalid).error == StageLoadError::DuplicateEnemyId, "duplicate monster id is rejected");
		invalid = stage;
		invalid.enemies[0].visual = static_cast<EnemyVisualKind>(999);
		Check(ValidateStageDefinition(invalid).error == StageLoadError::InvalidEnemyVisual, "unknown monster visual is rejected");
		invalid = stage;
		invalid.enemies.push_back({ "fourth", "Fourth", EnemyVisualKind::CrystalToad, 1.0f });
		Check(ValidateStageDefinition(invalid).error == StageLoadError::TooManyEnemies, "monster roster limit is enforced");
	}

	void TestEnemyDistanceAndAttackRanges()
	{
		StageDefinition stage = CreateDefaultStageDefinition();
		stage.id = "enemy-range-test";
		stage.rules.playerHealth = 100.0f;
		stage.rules.playerDamage = 10.0f;
		stage.rules.enemyStepsBeforeAttack = 4;
		stage.rules.enemyStep = 20.0f;
		stage.enemies = {
			{ "range-toad", "Range Toad", EnemyVisualKind::CrystalToad, 100.0f, 1.0f, 1 },
			{ "range-bat", "Range Bat", EnemyVisualKind::EmberBat, 100.0f, 1.0f, 3 },
			{ "range-shaman", "Range Shaman", EnemyVisualKind::MossShaman, 100.0f, 1.0f, 2 }
		};
		Check(ValidateStageDefinition(stage).IsValid(), "per-monster range stage validates");

		GameWorld world(stage);
		const auto initialEnemies = world.GetEnemies();
		Check(initialEnemies[0].distanceToPlayerCells == 4, "front monster starts at the stage distance");
		Check(initialEnemies[1].distanceToPlayerCells == 5, "second monster starts one cell behind");
		Check(initialEnemies[2].distanceToPlayerCells == 6, "third monster starts two cells behind");
		Check(initialEnemies[0].definition.attackRangeCells == 1, "melee monster keeps its one-cell range");
		Check(initialEnemies[1].definition.attackRangeCells == 3, "ranged monster keeps its three-cell range");
		Check(initialEnemies[2].definition.attackRangeCells == 2, "mid-range monster keeps its two-cell range");

		auto ResolveEmptyTurn = [](GameWorld& turnWorld)
		{
			Launch(turnWorld);
			turnWorld.GetBall().SetPosition({ 500.0f, 801.0f });
			turnWorld.Update(0.0f);
			turnWorld.Update(0.0f);
			return turnWorld.ConsumeEvents();
		};
		auto CountEvents = [](const std::vector<GameEvent>& events, GameEventType type)
		{
			return static_cast<int>(std::count_if(
				events.begin(),
				events.end(),
				[type](const GameEvent& event) { return event.type == type; }));
		};

		const std::vector<GameEvent> firstTurn = ResolveEmptyTurn(world);
		Check(CountEvents(firstTurn, GameEventType::EnemyAdvanced) == 3, "every living monster advances one cell after the first player attack");
		Check(CountEvents(firstTurn, GameEventType::PlayerDamaged) == 0, "out-of-range monsters do not attack");
		for (std::size_t index = 0; index < world.GetEnemies().size(); ++index)
		{
			Check(
				world.GetEnemies()[index].distanceToPlayerCells
					== initialEnemies[index].distanceToPlayerCells - 1,
				"one turn subtracts exactly one distance cell from each monster");
			Check(
				Near(world.GetEnemies()[index].actor.GetX(), initialEnemies[index].actor.GetX() - 20.0f),
				"one-cell movement uses the configured visual cell width");
		}

		const std::vector<GameEvent> secondTurn = ResolveEmptyTurn(world);
		Check(CountEvents(secondTurn, GameEventType::EnemyAdvanced) == 3, "monsters move on the turn they enter attack range");
		Check(CountEvents(secondTurn, GameEventType::PlayerDamaged) == 0, "range entry does not also attack in the same turn");
		Check(world.GetNextEnemyAction(1).type == EnemyActionType::Strike, "three-cell monster previews an attack after entering range");

		const float batXInRange = world.GetEnemies()[1].actor.GetX();
		const std::vector<GameEvent> thirdTurn = ResolveEmptyTurn(world);
		Check(CountEvents(thirdTurn, GameEventType::EnemyAdvanced) == 2, "only monsters still outside range advance");
		Check(CountEvents(thirdTurn, GameEventType::PlayerDamaged) == 1, "the first in-range monster attacks alone");
		Check(Near(world.GetPlayer().GetHp(), 90.0f), "one ranged monster deals one attack of damage");
		Check(Near(world.GetEnemies()[1].actor.GetX(), batXInRange), "an in-range monster stops moving while attacking");

		const std::vector<GameEvent> fourthTurn = ResolveEmptyTurn(world);
		Check(CountEvents(fourthTurn, GameEventType::EnemyAdvanced) == 1, "last out-of-range monster continues advancing alone");
		Check(CountEvents(fourthTurn, GameEventType::PlayerDamaged) == 2, "two in-range monsters attack on the same turn");
		Check(Near(world.GetPlayer().GetHp(), 70.0f), "simultaneous in-range attacks accumulate damage");

		const std::vector<GameEvent> fifthTurn = ResolveEmptyTurn(world);
		Check(CountEvents(fifthTurn, GameEventType::EnemyAdvanced) == 0, "no monster moves after every monster reaches range");
		Check(CountEvents(fifthTurn, GameEventType::PlayerDamaged) == 3, "every in-range monster attacks independently");
		Check(Near(world.GetPlayer().GetHp(), 40.0f), "three monster attacks accumulate deterministically");

		world.ResetGame();
		Check(world.GetEnemies()[0].distanceToPlayerCells == 4, "retry restores the first monster distance");
		Check(world.GetEnemies()[1].distanceToPlayerCells == 5, "retry restores the second monster distance");
		Check(world.GetEnemies()[2].distanceToPlayerCells == 6, "retry restores the third monster distance");
		Check(world.GetEnemies()[1].attackActionCount == 0, "retry clears per-monster attack pattern progress");
	}

	void TestCombinedSprintFiveContentRegression()
	{
		const std::filesystem::path repositoryRoot =
			std::filesystem::path(__FILE__).parent_path().parent_path();
		const ContentLoadResult content = LoadContentCatalog(
			repositoryRoot / "FinalProject_Peglin" / "content" / "stages.v1.ini");
		const StageDefinition* boss = FindContentStage(content.stages, "stage-3");
		Check(content.UsedExternalContent() && boss != nullptr, "combined regression starts from shipped external boss content");
		if (boss == nullptr)
		{
			return;
		}

		GameWorld world;
		Check(world.SelectOrb("echo-orb"), "combined regression selects echo orb");
		Check(world.AcquireRelic("combo-lantern"), "combined regression acquires score relic");
		Check(world.AcquireRelic("thorn-charm"), "combined regression acquires damage relic");
		Check(world.AcquireRelic("bark-guard"), "combined regression acquires defense relic");
		Check(world.LoadStage(*boss, GameDifficulty::Normal), "combined regression loads external boss stage");
		const ProgressionModifiers modifiers = world.GetProgressionModifiers();
		Check(Near(modifiers.pegDamageMultiplier, 0.96f), "combined orb and relic damage order is deterministic");
		Check(Near(modifiers.scoreMultiplier, 1.875f), "combined orb and relic score order is deterministic");
		Check(Near(modifiers.incomingDamageMultiplier, 0.85f), "combined defense relic multiplier is deterministic");
		Check(world.GetNextEnemyAction().type == EnemyActionType::Advance, "external boss begins outside its attack range");

		world.GetEnemy().SetHp(1.0f);
		Launch(world);
		auto scan = world.GetTargets()._targetBallList.GetHeadPosition();
		Vector2 criticalPeg;
		while (scan != nullptr)
		{
			const TargetBall& candidate = world.GetTargets()._targetBallList.GetNext(scan);
			if (candidate.type == PegType::Critical)
			{
				criticalPeg = candidate.position;
				break;
			}
		}
		world.GetBall().SetPosition(criticalPeg + Vector2{ -15.0f, 0.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		const std::vector<GameEvent> hitEvents = world.ConsumeEvents();
		Check(!hitEvents.empty() && Near(hitEvents.front().damage, 1.92f), "combined critical damage uses orb then relic multiplier");
		Check(world.GetScore().currentShot == 375, "combined critical score uses orb and relic multiplier");
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		Check(world.Update(0.0f) == GameUpdateResult::Victory, "combined external content reaches Victory");
		Check(world.GetResultSummary().has_value(), "combined victory produces a result summary");
		Check(world.Update(0.0f) == GameUpdateResult::None, "combined victory result remains single-shot");

		world.ResetGame();
		Check(world.GetState() == GameState::Aiming, "combined victory retry returns to aiming");
		Check(Near(world.GetEnemy().GetHp(), 60.0f), "combined retry restores external boss health");
		Check(world.GetLoadout().GetSelectedOrbId() == "echo-orb", "combined retry preserves selected orb");
		Check(world.GetLoadout().GetRelicStackCount("bark-guard") == 1, "combined retry preserves relics");
		Check(world.GetEnemy().GetCount() == 0 && Near(world.GetEnemyShield(), 0.0f), "combined retry resets boss action and shield state");

		GameUpdateResult defeatResult = GameUpdateResult::None;
		for (int turn = 0; turn < 20 && world.GetState() != GameState::Defeat; ++turn)
		{
			world.BeginAim({ 100.0f, 100.0f });
			world.ReleaseShot({ 90.0f, 100.0f });
			world.GetBall().SetPosition({ 500.0f, 801.0f });
			world.Update(0.0f);
			defeatResult = world.Update(0.0f);
		}
		Check(defeatResult == GameUpdateResult::Defeat, "combined boss actions reach Defeat deterministically");
		Check(world.GetState() == GameState::Defeat, "combined defeat enters terminal state");
		Check(world.GetEnemy().GetCount() == 11, "combined defeat occurs on the expected range-based boss turn");
		Check(world.Update(0.0f) == GameUpdateResult::None, "combined defeat result remains single-shot");
		world.ResetGame();
		Check(Near(world.GetPlayer().GetHp(), 110.0f), "combined defeat retry restores external player health");
		Check(world.GetLoadout().GetAcquiredRelics().size() == 3, "combined defeat retry preserves the complete loadout");
		world.ResetProgression();
		Check(world.GetLoadout().GetSelectedOrbId() == "basic-orb", "combined new progression restores basic orb");
		Check(world.GetLoadout().GetAcquiredRelics().empty(), "combined new progression clears all relics");
	}

	void TestStageVictoryAndDefeatRegression()
	{
		StageDefinition victoryStage = CreateDefaultStageDefinition();
		victoryStage.id = "victory-stage";
		victoryStage.pegLayout.pegs = { { { 500.0f, 500.0f }, PegType::Critical } };
		victoryStage.rules.playerHealth = 50.0f;
		victoryStage.rules.enemyHealth = 2.0f;
		victoryStage.rules.playerDamage = 10.0f;
		victoryStage.rules.enemyStepsBeforeAttack = 1;
		Check(ValidateStageDefinition(victoryStage).IsValid(), "victory regression stage validates");

		GameWorld victoryWorld(victoryStage);
		Launch(victoryWorld);
		victoryWorld.GetBall().SetPosition({ 485.0f, 500.0f });
		victoryWorld.GetBall().SetVelocity({ 2.0f, 0.0f });
		victoryWorld.Update(0.0f);
		victoryWorld.GetBall().SetPosition({ 500.0f, 801.0f });
		victoryWorld.Update(0.0f);
		Check(victoryWorld.Update(0.0f) == GameUpdateResult::Victory, "stage critical damage triggers Victory");
		Check(victoryWorld.GetState() == GameState::Victory, "stage victory enters terminal state");
		Check(victoryWorld.Update(0.0f) == GameUpdateResult::None, "stage victory result remains single-shot");

		StageDefinition defeatStage = victoryStage;
		defeatStage.id = "defeat-stage";
		defeatStage.rules.playerHealth = 10.0f;
		defeatStage.rules.enemyHealth = 20.0f;
		GameWorld defeatWorld(defeatStage);
		defeatWorld.GetEnemy().SetCount(1);
		Launch(defeatWorld);
		defeatWorld.GetBall().SetPosition({ 500.0f, 801.0f });
		defeatWorld.Update(0.0f);
		Check(defeatWorld.Update(0.0f) == GameUpdateResult::Defeat, "stage attack damage triggers Defeat");
		Check(defeatWorld.GetState() == GameState::Defeat, "stage defeat enters terminal state");
		Check(defeatWorld.Update(0.0f) == GameUpdateResult::None, "stage defeat result remains single-shot");
	}

	void TestOrbAndRelicProgression()
	{
		Check(GetOrbDefinitions().size() == 3, "orb catalog exposes three stable definitions");
		Check(GetRelicDefinitions().size() == 3, "relic catalog exposes three stable definitions");
		Check(FindOrbDefinition("basic-orb") != nullptr, "basic orb has a stable id");
		Check(FindOrbDefinition("unknown-orb") == nullptr, "unknown orb id is rejected");
		Check(FindRelicDefinition("combo-lantern") != nullptr, "combo lantern has a stable id");
		Check(FindRelicDefinition("unknown-relic") == nullptr, "unknown relic id is rejected");
		Check(FindOrbDefinition("basic-orb")->imageKey == "orb-traveler-v1", "traveler orb exposes a stable image key");
		Check(FindOrbDefinition("iron-orb")->imageKey == "orb-iron-v1", "iron orb exposes a stable image key");
		Check(FindOrbDefinition("echo-orb")->imageKey == "orb-echo-v1", "echo orb exposes a stable image key");
		Check(FindRelicDefinition("combo-lantern")->imageKey == "relic-combo-lantern-v1", "combo lantern exposes a stable image key");
		Check(FindRelicDefinition("thorn-charm")->imageKey == "relic-thorn-charm-v1", "thorn charm exposes a stable image key");
		Check(FindRelicDefinition("bark-guard")->imageKey == "relic-bark-guard-v1", "bark guard exposes a stable image key");

		const std::string ironDescription = DescribeOrbEffect(*FindOrbDefinition("iron-orb"));
		Check(ironDescription.find("페그 피해 ×1.50 (+50%)") != std::string::npos,
			"orb reward description reports the exact damage multiplier and percentage");
		Check(ironDescription.find("획득 점수 ×0.75 (-25%)") != std::string::npos,
			"orb reward description reports the exact score tradeoff");
		Check(ironDescription.find("근접 · 단일 대상") != std::string::npos,
			"orb reward description reports delivery and target type");
		const std::string barkDescription = DescribeRelicEffect(*FindRelicDefinition("bark-guard"));
		Check(barkDescription.find("받는 피해 ×0.85 (-15%)") != std::string::npos,
			"relic reward description reports its per-stack defense precisely");
		Check(barkDescription.find("2중첩 시 받는 피해 ×0.7225 (-27.75%)") != std::string::npos,
			"stackable relic description reports multiplicative maximum-stack defense");
		Check(DescribeRewardEffect({ RunRewardKind::Heal, {}, "Heal 25 HP", 25.0f })
			.find("최대 체력을 초과하지 않음") != std::string::npos,
			"healing reward description states its health cap behavior");

		PlayerLoadout loadout;
		Check(loadout.GetSelectedOrbId() == "basic-orb", "loadout starts with the basic orb");
		Check(loadout.GetOwnedOrbs().size() == 3, "loadout exposes the three starter orbs");
		Check(!loadout.GetNextOrbId().empty(), "loadout previews a next usable orb");
		Check(loadout.GetAcquiredRelics().empty(), "loadout starts without relics");
		Check(!loadout.SelectOrb("unknown-orb"), "loadout rejects an unknown orb");
		Check(loadout.GetSelectedOrbId() == "basic-orb", "failed orb selection preserves the current orb");
		Check(loadout.SelectOrb("iron-orb"), "loadout selects a registered orb");
		Check(loadout.GetSelectedOrbId() == "iron-orb", "selected orb id is retained");

		PlayerLoadout firstDeck;
		PlayerLoadout secondDeck;
		firstDeck.BeginBattle(20260831u);
		secondDeck.BeginBattle(20260831u);
		Check(firstDeck.GetSelectedOrbId() == secondDeck.GetSelectedOrbId(), "same seed reproduces the current orb");
		Check(firstDeck.GetNextOrbId() == secondDeck.GetNextOrbId(), "same seed reproduces the next orb preview");
		std::vector<std::string> firstCycle;
		for (std::size_t index = 0; index < firstDeck.GetOwnedOrbs().size(); ++index)
		{
			firstCycle.emplace_back(firstDeck.GetSelectedOrbId());
			Check(firstDeck.GetReloadCount() == 0, "deck does not refill before every owned orb is used");
			Check(firstDeck.AdvanceOrb(), "using an orb advances the deck");
		}
		std::vector<std::string> ownedCycle = firstDeck.GetOwnedOrbs();
		std::sort(firstCycle.begin(), firstCycle.end());
		std::sort(ownedCycle.begin(), ownedCycle.end());
		Check(firstCycle == ownedCycle, "one deck cycle uses every owned orb exactly once");
		Check(firstDeck.GetReloadCount() == 1, "deck refills after every owned orb is used");
		Check(!firstDeck.GetNextOrbId().empty(), "next orb preview remains available after refill");
		Check(!firstDeck.AddOrb("unknown-orb"), "deck rejects an unknown reward orb");
		const std::size_t ownedBeforeReward = firstDeck.GetOwnedOrbs().size();
		Check(firstDeck.AddOrb("iron-orb"), "deck accepts a registered reward orb");
		Check(firstDeck.GetOwnedOrbs().size() == ownedBeforeReward + 1, "reward orb is added to the owned list");

		Check(loadout.AcquireRelic("combo-lantern"), "unique relic can be acquired once");
		Check(!loadout.AcquireRelic("combo-lantern"), "unique relic rejects a duplicate");
		Check(loadout.AcquireRelic("bark-guard"), "stackable relic accepts its first stack");
		Check(loadout.AcquireRelic("bark-guard"), "stackable relic accepts its second stack");
		Check(!loadout.AcquireRelic("bark-guard"), "stackable relic enforces its maximum stacks");
		Check(!loadout.AcquireRelic("unknown-relic"), "loadout rejects an unknown relic");
		Check(loadout.GetRelicStackCount("combo-lantern") == 1, "unique relic reports one stack");
		Check(loadout.GetRelicStackCount("bark-guard") == 2, "stackable relic reports two stacks");

		const ProgressionModifiers modifiers = loadout.CalculateModifiers();
		Check(Near(modifiers.pegDamageMultiplier, 1.5f), "orb damage modifier applies before relics");
		Check(Near(modifiers.scoreMultiplier, 0.9375f), "orb and relic score modifiers compose deterministically");
		Check(Near(modifiers.incomingDamageMultiplier, 0.7225f), "relic acquisition order composes incoming damage");

		GameWorld world;
		Check(world.SelectOrb("iron-orb"), "game world accepts a registered orb");
		Check(world.AcquireRelic("bark-guard"), "game world acquires the first guard relic");
		Check(world.AcquireRelic("bark-guard"), "game world acquires the second guard relic");
		Launch(world);
		auto scan = world.GetTargets()._targetBallList.GetHeadPosition();
		Vector2 normalPeg;
		while (scan != nullptr)
		{
			const TargetBall& candidate = world.GetTargets()._targetBallList.GetNext(scan);
			if (candidate.type == PegType::Normal)
			{
				normalPeg = candidate.position;
				break;
			}
		}
		world.GetBall().SetPosition(normalPeg + Vector2{ -15.0f, 0.0f });
		world.GetBall().SetVelocity({ 2.0f, 0.0f });
		world.Update(0.0f);
		Check(world.GetScore().currentShot == 75, "iron orb reduces awarded score deterministically");
		Check(Near(world.ConsumeEvents().front().damage, 1.5f), "iron orb increases peg damage in feedback");
		world.GetBall().SetPosition({ 500.0f, 801.0f });
		world.Update(0.0f);
		world.Update(0.0f);
		Check(Near(world.GetEnemy().GetHp(), 18.5f), "iron orb damage reaches turn resolution");

		world.ResetGame();
		Check(world.GetLoadout().GetSelectedOrbId() == "iron-orb", "retry preserves the selected orb");
		Check(world.GetLoadout().GetRelicStackCount("bark-guard") == 2, "retry preserves acquired relics");
		for (int turn = 0; turn < 8; ++turn)
		{
			Launch(world);
			world.GetBall().SetPosition({ 500.0f, 801.0f });
			world.Update(0.0f);
			world.Update(0.0f);
		}
		Check(Near(world.GetFeedback().lastPlayerDamage, 14.45f), "two guard relics reduce incoming damage");
		Check(Near(world.GetPlayer().GetHp(), 85.55f), "reduced incoming damage updates player health");

		world.ResetProgression();
		Check(world.GetLoadout().GetSelectedOrbId() == "basic-orb", "progression reset restores the basic orb");
		Check(world.GetLoadout().GetAcquiredRelics().empty(), "progression reset removes acquired relics");
		Check(Near(world.GetProgressionModifiers().pegDamageMultiplier, 1.0f), "progression reset restores neutral damage");
		Check(Near(world.GetPlayer().GetHp(), 100.0f), "progression reset also starts a fresh game");
	}

	void TestAttackTypesAndTargets()
	{
		const OrbDefinition* traveler = FindOrbDefinition("basic-orb");
		const OrbDefinition* iron = FindOrbDefinition("iron-orb");
		const OrbDefinition* echo = FindOrbDefinition("echo-orb");
		Check(traveler != nullptr
			&& traveler->attackDelivery == AttackDelivery::Projectile
			&& traveler->attackTarget == AttackTarget::Single,
			"traveler orb defines a single projectile attack");
		Check(iron != nullptr
			&& iron->attackDelivery == AttackDelivery::Melee
			&& iron->attackTarget == AttackTarget::Single,
			"iron orb defines a single melee attack");
		Check(echo != nullptr
			&& echo->attackDelivery == AttackDelivery::Projectile
			&& echo->attackTarget == AttackTarget::All,
			"echo orb defines an all-target projectile attack");

		StageDefinition stage = CreateDefaultStageDefinition();
		stage.id = "attack-type-test";
		stage.enemies = {
			{ "target-a", "Target A", EnemyVisualKind::CrystalToad, 10.0f },
			{ "target-b", "Target B", EnemyVisualKind::EmberBat, 10.0f },
			{ "target-c", "Target C", EnemyVisualKind::MossShaman, 10.0f }
		};
		GameWorld world(stage);
		auto ResolveOneNormalPeg = [](GameWorld& targetWorld)
		{
			Launch(targetWorld);
			auto scan = targetWorld.GetTargets()._targetBallList.GetHeadPosition();
			Vector2 normalPeg;
			while (scan != nullptr)
			{
				const TargetBall& candidate = targetWorld.GetTargets()._targetBallList.GetNext(scan);
				if (candidate.type == PegType::Normal)
				{
					normalPeg = candidate.position;
					break;
				}
			}
			targetWorld.GetBall().SetPosition(normalPeg + Vector2{ -15.0f, 0.0f });
			targetWorld.GetBall().SetVelocity({ 2.0f, 0.0f });
			targetWorld.Update(0.0f);
			targetWorld.GetBall().SetPosition({ 500.0f, 801.0f });
			targetWorld.Update(0.0f);
			targetWorld.Update(0.0f);
			return targetWorld.ConsumeEvents();
		};
		auto FindAttack = [](const std::vector<GameEvent>& events)
		{
			return std::find_if(events.begin(), events.end(), [](const GameEvent& event)
			{
				return event.type == GameEventType::PlayerAttack;
			});
		};

		Check(world.SelectOrb("basic-orb"), "single projectile test selects traveler orb");
		world.ResetGame();
		const std::vector<GameEvent> travelerEvents = ResolveOneNormalPeg(world);
		Check(Near(world.GetEnemies()[0].actor.GetHp(), 9.0f), "single attack damages the active enemy");
		Check(Near(world.GetEnemies()[1].actor.GetHp(), 10.0f)
			&& Near(world.GetEnemies()[2].actor.GetHp(), 10.0f),
			"single attack leaves non-target enemies unchanged");
		const auto travelerAttack = FindAttack(travelerEvents);
		Check(travelerAttack != travelerEvents.end()
			&& travelerAttack->attackDelivery == AttackDelivery::Projectile
			&& travelerAttack->attackTarget == AttackTarget::Single
			&& travelerAttack->affectedPegs == 1,
			"single projectile event identifies one target");

		Check(world.SelectOrb("echo-orb"), "all-target test selects echo orb");
		world.ResetGame();
		const std::vector<GameEvent> echoEvents = ResolveOneNormalPeg(world);
		Check(Near(world.GetEnemies()[0].actor.GetHp(), 9.2f)
			&& Near(world.GetEnemies()[1].actor.GetHp(), 9.2f)
			&& Near(world.GetEnemies()[2].actor.GetHp(), 9.2f),
			"all-target attack damages every living enemy");
		const auto echoAttack = FindAttack(echoEvents);
		Check(echoAttack != echoEvents.end()
			&& echoAttack->attackDelivery == AttackDelivery::Projectile
			&& echoAttack->attackTarget == AttackTarget::All
			&& echoAttack->affectedPegs == 3,
			"all-target projectile event identifies the full roster");

		Check(world.SelectOrb("iron-orb"), "melee test selects iron orb");
		world.ResetGame();
		const std::vector<GameEvent> ironEvents = ResolveOneNormalPeg(world);
		Check(Near(world.GetEnemies()[0].actor.GetHp(), 8.5f), "melee attack applies iron orb damage to one target");
		const auto ironAttack = FindAttack(ironEvents);
		Check(ironAttack != ironEvents.end()
			&& ironAttack->attackDelivery == AttackDelivery::Melee
			&& ironAttack->attackTarget == AttackTarget::Single,
			"melee event remains distinct from projectile attacks");
	}
}

int main()
{
	TestZeroLengthShot();
	TestTerminalTransitionGate();
	TestWallReflection();
	TestPegReflectionAndSingleDamage();
	TestScoreComboProgression();
	TestScreenExitTurn();
	TestVictoryTransition();
	TestDefeatTransition();
	TestStateAndRestitutionRules();
	TestMouseUiNavigation();
	TestAdventureRunProgression();
	TestLayoutConfiguration();
	TestSeededPegLayout();
	TestDataDrivenPegLayout();
	TestMovingPegStages();
	TestPegTypeDefinitions();
	TestCriticalPegEffect();
	TestBombPegEffect();
	TestRefreshPegEffect();
	TestRefreshPegMinimumAfterTurn();
	TestStageCatalogAndValidation();
	TestStageSelectionAndResultSummary();
	TestGameOptionsAndDifficulty();
	TestSoftPegHitSound();
	TestGameSettingsPersistence();
	TestStageRecordPersistence();
	TestStageRulesConfigureWorld();
	TestScoreCancellationAndContinuation();
	TestBombDoesNotChainSecondaryEffects();
	TestRefreshDoesNotDuplicateActivePegs();
	TestGameEventFeedbackStream();
	TestAimPreviewMatchesShotInput();
	TestStageValidationMatrix();
	TestStageVictoryAndDefeatRegression();
	TestOrbAndRelicProgression();
	TestAttackTypesAndTargets();
	TestBossEnemyActionPattern();
	TestExternalContentCatalog();
	TestExternalGameplayCatalog();
	TestLocalizationCatalog();
	TestMultipleEnemyEncounter();
	TestEnemyDistanceAndAttackRanges();
	TestCombinedSprintFiveContentRegression();

	if (failures == 0)
	{
		std::cout << "All Peglin core tests passed.\n";
		return 0;
	}

	std::cerr << failures << " Peglin core test(s) failed.\n";
	return 1;
}
