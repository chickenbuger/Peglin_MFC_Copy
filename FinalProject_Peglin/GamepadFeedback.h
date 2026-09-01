#pragma once

enum class GamepadConnectionChange
{
	None,
	Connected,
	Reconnected,
	Disconnected
};

class GamepadConnectionTracker
{
public:
	GamepadConnectionChange Update(bool connected) noexcept;
	bool IsConnected() const noexcept { return _connected; }
	bool HasConnectedBefore() const noexcept { return _hasConnectedBefore; }

private:
	bool _connected = false;
	bool _hasConnectedBefore = false;
};

enum class GamepadRumbleCue
{
	PegHit,
	HeavyImpact,
	PlayerDamaged,
	Victory,
	Defeat
};

struct GamepadMotorLevels
{
	float left = 0.0f;
	float right = 0.0f;
};

class GamepadRumbleEnvelope
{
public:
	void Trigger(GamepadRumbleCue cue) noexcept;
	void Update(float deltaSeconds) noexcept;
	void Reset() noexcept;
	bool IsActive() const noexcept { return _remainingSeconds > 0.0f; }
	GamepadMotorLevels GetLevels() const noexcept;

private:
	float _leftMotor = 0.0f;
	float _rightMotor = 0.0f;
	float _durationSeconds = 0.0f;
	float _remainingSeconds = 0.0f;
	int _priority = 0;
};
