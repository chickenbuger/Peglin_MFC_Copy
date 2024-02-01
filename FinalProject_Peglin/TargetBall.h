#pragma once
class TargetBall
{
public:
	TargetBall() {}
	~TargetBall() {}
public:
	float x, y;
	float size = 10.0f;
public:
	void draw(CDC* pDC);
	void setting(float px, float py);
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