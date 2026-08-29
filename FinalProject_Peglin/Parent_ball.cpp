#include "pch.h"
#include "Parent_ball.h"
#include "Physics.h"
#include <algorithm>
#include <cmath>

constexpr int MAX_WIDTH = 970;
constexpr int MIN_WIDTH = 40;
constexpr int CEILING_HEIGHT = 205;
constexpr float INITIAL_X = 490.0f;
constexpr float INITIAL_Y = 250.0f;

constexpr float MIN_POWER = 1.0f;
constexpr float MAX_POWER = 400.0f;

constexpr float CONVERT_MIN_POWER = 1.0f;
constexpr float CONVERT_MAX_POWER = 10.0f;
constexpr float MIN_DRAG_DISTANCE = 0.001f;
constexpr float BASE_TIMESTEP_SECONDS = 0.01f;
constexpr float WALL_RESTITUTION = 1.0f;
constexpr float LEFT_BOUNDARY = 35.0f;
constexpr float RIGHT_BOUNDARY = 945.0f;
constexpr float TOP_BOUNDARY = 215.0f;

namespace
{
	int RoundToPixel(float value)
	{
		return static_cast<int>(std::lround(value));
	}
}

Parent_ball::Parent_ball() : _gravity(0.01f), IsActive(false), IsClick(false)
{
	_size = 10;
	Init();
}

bool Parent_ball::shooting()
{
	//힘에 대한 계산
	const Vector2 direction = _startDragPosition - _endDragPosition;
	float magnitude = direction.Length();
	if (magnitude <= MIN_DRAG_DISTANCE)
	{
		_velocity = {};
		_force = 0.0f;
		IsActive = false;
		return false;
	}

	_velocity = direction / magnitude;

	//힘의 값을 MinPower ~ MaxPower 사이로 조정
	magnitude = std::clamp(magnitude, MIN_POWER, MAX_POWER);

	//힘의 값을 1~10사이 값으로 고정
	magnitude = magnitude / (MAX_POWER / CONVERT_MAX_POWER);
	_force = std::clamp(magnitude, CONVERT_MIN_POWER, CONVERT_MAX_POWER);

	IsActive = true;
	return true;
}

void Parent_ball::draw(CDC* pDC)
{
	const int savedDc = pDC->SaveDC();

	CBrush brush(RGB(200, 200, 200));
	pDC->SelectObject(&brush);
	pDC->SelectObject(GetStockObject(NULL_PEN));
	pDC->Ellipse(
		RoundToPixel(_position.x - _size),
		RoundToPixel(_position.y - _size),
		RoundToPixel(_position.x + _size),
		RoundToPixel(_position.y + _size));
	
	CPen pen(PS_SOLID, 4, RGB(255, 255, 255));
	pDC->SelectObject(&pen);

	if (!IsActive && IsClick)
	{
		drawline(pDC);
	}

	pDC->RestoreDC(savedDc);
}

void Parent_ball::update(float deltaSeconds)
{
	if (IsActive && !stop)
	{
		const float timeScale = deltaSeconds / BASE_TIMESTEP_SECONDS;
		//공의 움직임
		movement(timeScale);
		//공의 충돌 확인
		collision();
	}
}

void Parent_ball::Init()
{
	_position = { INITIAL_X, INITIAL_Y };
	_startDragPosition = _position;
	_traceDragPosition = _position;
	_endDragPosition = _position;

	_force = 0.0f;
	_velocity = {};

	IsActive = false;
	IsClick = false;
	stop = false;
}

void Parent_ball::collision()
{
	if (_position.x < LEFT_BOUNDARY)
	{
		_position.x = LEFT_BOUNDARY;
		_velocity = ReflectVelocity(_velocity, { 1.0f, 0.0f }, WALL_RESTITUTION);
	}
	else if (_position.x > RIGHT_BOUNDARY)
	{
		_position.x = RIGHT_BOUNDARY;
		_velocity = ReflectVelocity(_velocity, { -1.0f, 0.0f }, WALL_RESTITUTION);
	}
	if (_position.y < TOP_BOUNDARY)
	{
		_position.y = TOP_BOUNDARY;
		_velocity = ReflectVelocity(_velocity, { 0.0f, 1.0f }, WALL_RESTITUTION);
	}
}

void Parent_ball::drawline(CDC* pDC)
{
	const Vector2 dragDirection = _traceDragPosition - _startDragPosition;
	float magnitude = dragDirection.Length();

	if (magnitude == 0) return;

	const Vector2 direction = dragDirection / magnitude;
	const float ratioX = direction.x;
	float ratioY = direction.y;

	//힘의 값을 MinPower ~ MaxPower 사이로 조정
	magnitude = std::clamp(magnitude, MIN_POWER, MAX_POWER);

	//힘의 값을 1~10사이 값으로 고정
	magnitude = magnitude / (MAX_POWER / CONVERT_MAX_POWER);
	magnitude = std::clamp(magnitude, CONVERT_MIN_POWER, CONVERT_MAX_POWER);

	//선의 좌표들
	float x1 = _position.x;
	float y1 = _position.y;

	float line_x = 1.0f;
	float line_y = 1.0f;

	//펜 선택
	CPen pen(PS_SOLID, 4, RGB(255, 255, 255));
	pDC->SelectObject(&pen);

	for (int i = 0; i < (int)magnitude; i++)
	{
		ratioY -= _gravity;

		float x2 = x1 - ratioX * magnitude * line_x;
		float y2 = y1 - ratioY * magnitude * line_y;

		//벽에 닿으면 x축이 반대로
		if ((x2 < MIN_WIDTH) || (x2 > MAX_WIDTH)) line_x *= -1;
		//천장에 닿으면 y축이 반대로
		if (y2 < CEILING_HEIGHT) line_y *= -1;

		pDC->MoveTo(RoundToPixel(x1), RoundToPixel(y1));
		pDC->LineTo(RoundToPixel(x2), RoundToPixel(y2));

		//new->old
		x1 = x2; y1 = y2;
	}
}

void Parent_ball::movement(float timeScale)
{
	_velocity.y += _gravity * timeScale;
	_position += _velocity * (_force * timeScale);

}
