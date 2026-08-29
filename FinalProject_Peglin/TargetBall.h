#pragma once

#include "Vector2.h"
class TargetBall
{
public:
	TargetBall() {}
	~TargetBall() {}
public:
	Vector2 position;
	float size = 10.0f;
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
