#pragma once

#include "GameLayout.h"
class TargetBall
{
public:
	TargetBall() {}
	~TargetBall() {}
public:
	Vector2 position;
	float size = GameLayout::PegRadius;
public:
	void draw(CDC* pDC);
	void setting(Vector2 newPosition);
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
