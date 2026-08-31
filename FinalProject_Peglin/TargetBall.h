#pragma once

#include "GameLayout.h"
#include "GameOptions.h"
#include "PegLayout.h"

#include <cstddef>

class TargetBall
{
public:
	TargetBall() {}
	~TargetBall() {}
public:
	Vector2 position;
	float size = GameLayout::PegRadius;
	PegType type = PegType::Normal;
public:
	void draw(CDC* pDC, PegColorMode colorMode = PegColorMode::Standard);
	void setting(const PegDefinition& definition, std::size_t sourceIndex = 0);
	void UpdateMotion(float elapsedSeconds) noexcept;
	Vector2 PositionAt(float elapsedSeconds) const noexcept;
	bool IsMoving() const noexcept { return _motion.IsMoving(); }
	std::size_t GetSourceIndex() const noexcept { return _sourceIndex; }
	const Vector2& GetOrigin() const noexcept { return _origin; }
	const PegMotionDefinition& GetMotion() const noexcept { return _motion; }

private:
	Vector2 _origin;
	PegMotionDefinition _motion;
	std::size_t _sourceIndex = 0;
};

class TargetBallList
{
public:
	CList<TargetBall> _targetBallList;
public:
	TargetBallList() {}
	~TargetBallList() {}
public:
	inline void add(TargetBall& b) { _targetBallList.AddTail(b); }
};
