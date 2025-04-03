#include "pch.h"
#include "Parent_ball.h"
#include <stdlib.h>

constexpr int MAX_WIDTH = 970;
constexpr int MIN_WIDTH = 40;
constexpr int CEILING_HEIGHT = 205;

Parent_ball::Parent_ball()
{
	pos[0] = 490.0f;
	pos[1] = 250.0f;
	_size = 10;
	_force_x = _force_y = _force = 15;
	_gravity = 1.6f;
	_gravity_mul = 0.06f;

	_line_x = 1.0f;
	_line_y = 1.0f;

	IsActive = false;
	IsClick = false;

	IsCrashToTargetball = false;
}

void Parent_ball::shooting()
{
	IsActive = true;
}

void Parent_ball::draw(CDC* pDC)
{
	CBrush brush(RGB(200.0f, 200.0f, 200.0f));
	pDC->SelectObject(brush);
	pDC->SelectObject(GetStockObject(NULL_PEN));
	pDC->Ellipse(pos[0] - _size, pos[1] - _size, pos[0] + _size, pos[1] + _size);
	
	CPen pen(PS_SOLID, 4, RGB(255.0f, 255.0f, 255.0f));
	pDC->SelectObject(pen);

	if (IsActive) return;

	if (IsClick)
	{
		float r = sqrt(pow(Fpos[0] - CheckPos[0], 2) + pow(Fpos[1] - CheckPos[1], 2));
		float dx = (CheckPos[0] - Fpos[0]) / r * 5.3f;
		float dy = (CheckPos[1] - Fpos[1]) / r * 5.3f;

		// 조준선 시작 위치
		float x1 = pos[0];
		float y1 = pos[1];
		float x2 = x1 - dx;
		float y2 = y1 - dy + (_gravity * (_gravity_mul * 1.1)) / 15.0f;

		// 벽 충돌 감지 방향 변수
		_line_x = 1;
		_line_y = 1;

		CPen pen(PS_SOLID, 4, RGB(255, 255, 255));
		pDC->SelectObject(pen);

		//라인 그리기
		for (int i = 1; i < 20; i++)
		{
			pDC->MoveTo(x1, y1);

			// 벽 충돌 감지 후 방향 반전
			if ((x2 < MIN_WIDTH) || (x2 > MAX_WIDTH)) _line_x *= -1;
			if (y2 < CEILING_HEIGHT) _line_y *= -1;

			pDC->LineTo(x2, y2);
			x1 = x2;
			y1 = y2;
			x2 = x1 - dx * _line_x;
			y2 = y1 - dy * _line_y + (_gravity * (_gravity_mul * 1.1 * i)) / 15.0f;
		}
	}
}

void Parent_ball::update(float delta)
{
	if (IsActive)
	{
		if (stop == false)
		{
			float r = sqrt(pow((Fpos[0] - Spos[0]), 2) + pow((Fpos[1] - Spos[1]), 2));
			_Current_Dir_x = (Fpos[0] - Spos[0]) / r * _force_x;
			_Current_Dir_y = (Fpos[1] - Spos[1]) / r * _force_y + _gravity * _gravity_mul;

			pos[0] = pos[0] + _Current_Dir_x;
			pos[1] = pos[1] + _Current_Dir_y;
			_gravity_mul *= 1.1;
			
			collision();
		}
	}
}

void Parent_ball::Init()
{
	pos[0] = 490.0f;
	pos[1] = 250.0f;
	IsActive = false;
	stop = false;
	_force_x = 15;
	_force_y = 15;
	_gravity_mul = 0.06f;
}

void Parent_ball::collision()
{
	if ((pos[0] < 35) || (pos[0] > 945))
	{
		_force_x *= -1;
	}
	if (pos[1] < 215)
	{
		_force_y *= -1;
	}
}