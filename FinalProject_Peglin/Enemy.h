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
	void SetCount(int count)	{ this->count = count; }
	void SetHp(float hp)		{ this->hp = hp; }
	void SetX(float x)			{ this->x = x; }

	//getter
	int		GetCount()	{ return count; }
	float	GetHp()		{ return hp; }
	float	GetX()		{ return x; }
	float	GetInit()	{ return Init_x; }
public:
	void draw(CDC* pDC);
	void Init();
};