#ifndef __PLAYER_H__
#define __PLAYER_H__

#pragma once
class Player
{
public:
	Player() {}
	~Player() {}
//변수 입니다.
private:
	float	hp = 100.0f;

//함수 입니다.
public:
	//setter
	void SetHp(float hp) { this->hp = hp; }
	
	//getter
	float GetHp() { return hp; }
	 
public:
	void draw(CDC* pDC);
	void Init();
};

#endif