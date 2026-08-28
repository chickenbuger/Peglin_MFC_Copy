#pragma once
class Enemy
{
public:
	Enemy() {}
	~Enemy() {}
private:
	int count = 0;
	float hp = 20.0f;
	float Init_x = 710.0f;
	float x = 710.0f;
public:
	//setter
	void SetCount(int InCount)	{ this->count = InCount; }
	void SetHp(float InHp)		{ this->hp = InHp; }
	void SetX(float x)			{ this->x = x; }

	//getter
	int		GetCount()	{ return count; }
	float	GetHp()		{ return hp; }
	float	GetX()		{ return x; }
public:
	void draw(CDC* pDC);
	void Init();
};
