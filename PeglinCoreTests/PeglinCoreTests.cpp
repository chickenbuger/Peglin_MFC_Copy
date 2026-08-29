#include <afxwin.h>

#include <cmath>
#include <iostream>
#include <string_view>

#include "GameWorld.h"
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
		Check(Near(ball.GetPosition().x, 35.0f), "left wall corrects overlap");
		Check(Near(ball.GetVelocity().x, 2.0f), "left wall reflects approaching velocity");
		Check(Near(ball.GetVelocity().y, 1.0f), "left wall preserves tangent velocity");

		ball.SetPosition({ 500.0f, 214.0f });
		ball.SetVelocity({ 1.0f, -3.0f });
		ball.collision();
		Check(Near(ball.GetPosition().y, 215.0f), "ceiling corrects overlap");
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
		Check(world.GetState() == GameState::Aiming, "ordinary turn returns to Aiming");
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
		Check(Near(world.GetEnemy().GetX(), 646.0f), "screen exit advances enemy position");
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
}

int main()
{
	TestZeroLengthShot();
	TestWallReflection();
	TestPegReflectionAndSingleDamage();
	TestScreenExitTurn();
	TestVictoryTransition();
	TestDefeatTransition();
	TestStateAndRestitutionRules();

	if (failures == 0)
	{
		std::cout << "All Peglin core tests passed.\n";
		return 0;
	}

	std::cerr << failures << " Peglin core test(s) failed.\n";
	return 1;
}
