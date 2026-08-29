#pragma once

#include "GameLayout.h"
#include "GameOptions.h"
#include "PegLayout.h"
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
	void setting(const PegDefinition& definition);
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
