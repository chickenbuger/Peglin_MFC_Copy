#ifndef __PLAYER_H__
#define __PLAYER_H__

#pragma once
class Player
{
public:
	Player() {}
	~Player() {}
//���� �Դϴ�.
private:
	float	hp = 100.0f;

//�Լ� �Դϴ�.
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