#pragma once

#include "GameLayout.h"

class Enemy
{
public:
	Enemy() {}
	~Enemy() {}
private:
	int count = 0;
	float hp = 20.0f;
	float x = GameLayout::EnemyInitialPosition.x;
public:
	//setter
	void SetCount(int InCount)	{ this->count = InCount; }
	void SetHp(float InHp)		{ this->hp = InHp; }
	void SetX(float newX)		{ x = newX; }

	//getter
	int		GetCount() const noexcept	{ return count; }
	float	GetHp() const noexcept		{ return hp; }
	float	GetX() const noexcept		{ return x; }
public:
	void draw(
		CDC* pDC,
		CBitmap* sprite = nullptr,
		Vector2 drawSize = GameLayout::EnemySize,
		float drawY = GameLayout::EnemyInitialPosition.y,
		bool activeTarget = false) const;
	void Init();
};
