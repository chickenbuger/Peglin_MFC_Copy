#include "pch.h"
#include "ball_liner.h"

void ball_liner::draw(CDC* pDC)
{
	CBrush brush(RGB(255.0f, 255.0f, 255.0f));
	pDC->SelectObject(brush);

	pDC->Ellipse(pos[0] - _size, pos[1] - _size, pos[0] + _size, pos[1] + _size);
}

void ball_liner::update(float delta)
{
	_lifetime -= delta;

	float r = sqrt(pow((Fpos[0] - CheckPos[0]), 2) + pow((Fpos[1] - CheckPos[1]), 2));
	pos[0] = pos[0] + (Fpos[0] - CheckPos[0]) / r * _force;
	pos[1] = pos[1] + (Fpos[1] - CheckPos[1]) / r * _force + _gravity * _gravity_mul;
	_gravity_mul *= 1.07;
}

bool ball_liner::isDead()
{	
	if (_lifetime < 0.0f)
	{
		return true;
	}
	return false;
}
