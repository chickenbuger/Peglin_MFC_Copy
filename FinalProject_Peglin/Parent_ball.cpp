#include "pch.h"
#include "Parent_ball.h"
#include <algorithm>
#include <iostream>

constexpr int MAX_WIDTH = 970;
constexpr int MIN_WIDTH = 40;
constexpr int CEILING_HEIGHT = 205;

constexpr float MIN_POWER = 1.0f;
constexpr float MAX_POWER = 400.0f;

constexpr float CONVERT_MIN_POWER = 1.0f;
constexpr float CONVERT_MAX_POWER = 10.0f;

Parent_ball::Parent_ball() : _gravity(0.01f), IsActive(false), IsClick(false)
{
	pos[0] = 490.0f;
	pos[1] = 250.0f;
	_size = 10;

	IsCrashToTargetball = false;
}

void Parent_ball::shooting()
{
	//힘에 대한 계산
	double dirx = StartDragPos[0] - EndDragPos[0];
	double diry = StartDragPos[1] - EndDragPos[1];

	float magnitude = sqrt(dirx * dirx + diry * diry);

	_velocity_x = dirx / magnitude;
	_velocity_y = diry / magnitude;

	//힘의 값을 MinPower ~ MaxPower 사이로 조정
	magnitude = std::clamp(magnitude, MIN_POWER, MAX_POWER);

	//힘의 값을 1~10사이 값으로 고정
	magnitude = magnitude / (MAX_POWER / CONVERT_MAX_POWER);
	_force = std::clamp(magnitude, CONVERT_MIN_POWER, CONVERT_MAX_POWER);

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
		drawline(pDC);
	}
}

void Parent_ball::update()
{
	if (IsActive && !stop)
	{
		//공의 움직임
		movement();
		//공의 충돌 확인
		collision();
	}
}

void Parent_ball::Init()
{
	pos[0] = 490.0f;
	pos[1] = 250.0f;

	IsActive = false;
	stop = false;
}

void Parent_ball::collision()
{
	if ((pos[0] < 35) || (pos[0] > 945))
	{
		_velocity_x *= -1;
	}
	if (pos[1] < 215)
	{
		_velocity_y *= -1;
	}
}

void Parent_ball::drawline(CDC* pDC)
{
	double dirx = TraceDragPos[0] - StartDragPos[0];
	double diry = TraceDragPos[1] - StartDragPos[1];

	float magnitude = sqrt(dirx * dirx + diry * diry);

	if (magnitude == 0) return;

	float ratioX = dirx / magnitude;
	float ratioY = diry / magnitude;

	//힘의 값을 MinPower ~ MaxPower 사이로 조정
	magnitude = std::clamp(magnitude, MIN_POWER, MAX_POWER);

	//힘의 값을 1~10사이 값으로 고정
	magnitude = magnitude / (MAX_POWER / CONVERT_MAX_POWER);
	magnitude = std::clamp(magnitude, CONVERT_MIN_POWER, CONVERT_MAX_POWER);

	//선의 좌표들
	float x1 = pos[0];
	float y1 = pos[1];

	float line_x = 1.0f;
	float line_y = 1.0f;

	//펜 선택
	CPen pen(PS_SOLID, 4, RGB(255, 255, 255));
	pDC->SelectObject(pen);

	for (int i = 0; i < (int)magnitude; i++)
	{
		ratioY += _gravity;

		float x2 = x1 - ratioX * magnitude * line_x;
		float y2 = y1 - ratioY * magnitude * line_y;

		//벽에 닿으면 x축이 반대로
		if ((x2 < MIN_WIDTH) || (x2 > MAX_WIDTH)) line_x *= -1;
		//천장에 닿으면 y축이 반대로
		if (y2 < CEILING_HEIGHT) line_y *= -1;

		pDC->MoveTo(x1, y1);
		pDC->LineTo(x2, y2);

		//new->old
		x1 = x2; y1 = y2;
	}
}

void Parent_ball::movement()
{
	_velocity_y += _gravity;
	pos[0] = pos[0] + _velocity_x * _force;
	pos[1] = pos[1] + _velocity_y * _force;
}
