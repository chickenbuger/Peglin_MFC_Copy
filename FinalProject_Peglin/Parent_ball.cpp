#include "pch.h"
#include "Parent_ball.h"
#include "GameLayout.h"
#include "Physics.h"
#include <algorithm>
#include <cmath>

constexpr float MIN_POWER = 1.0f;
constexpr float MAX_POWER = 400.0f;

constexpr float CONVERT_MIN_POWER = 1.0f;
constexpr float CONVERT_MAX_POWER = 10.0f;
constexpr float MIN_DRAG_DISTANCE = 0.001f;
constexpr float BASE_TIMESTEP_SECONDS = 0.01f;
constexpr float WALL_RESTITUTION = 1.0f;

namespace
{
	int RoundToPixel(float value)
	{
		return static_cast<int>(std::lround(value));
	}
}

Parent_ball::Parent_ball() : _gravity(0.01f), IsActive(false), IsClick(false)
{
	_size = GameLayout::BallRadius;
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

void Parent_ball::draw(CDC* pDC, CBitmap* sprite, float visualOffsetY, float visualScale)
{
	const int savedDc = pDC->SaveDC();
	constexpr float VISUAL_RADIUS = 16.0f;
	const float visualRadius = VISUAL_RADIUS * visualScale;
	const float visualCenterY = _position.y + visualOffsetY;

	if (sprite != nullptr && sprite->GetSafeHandle() != nullptr)
	{
		CDC spriteDc;
		spriteDc.CreateCompatibleDC(pDC);
		CBitmap* previousBitmap = spriteDc.SelectObject(sprite);
		BITMAP bitmapInfo{};
		sprite->GetBitmap(&bitmapInfo);
		::TransparentBlt(
			pDC->GetSafeHdc(),
			RoundToPixel(_position.x - visualRadius),
			RoundToPixel(visualCenterY - visualRadius),
			RoundToPixel(visualRadius * 2.0f),
			RoundToPixel(visualRadius * 2.0f),
			spriteDc.GetSafeHdc(),
			0,
			0,
			bitmapInfo.bmWidth,
			bitmapInfo.bmHeight,
			RGB(255, 0, 255));
		spriteDc.SelectObject(previousBitmap);
	}
	else
	{
		CBrush brush(RGB(200, 200, 200));
		CBrush* previousBrush = pDC->SelectObject(&brush);
		CPen* previousPen = static_cast<CPen*>(
			pDC->SelectStockObject(NULL_PEN));
		pDC->Ellipse(
			RoundToPixel(_position.x - _size),
			RoundToPixel(_position.y - _size),
			RoundToPixel(_position.x + _size),
			RoundToPixel(_position.y + _size));
		pDC->SelectObject(previousPen);
		pDC->SelectObject(previousBrush);
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
	_position = GameLayout::BallInitialPosition;
	_startDragPosition = _position;
	_endDragPosition = _position;

	_force = 0.0f;
	_velocity = {};

	IsActive = false;
	IsClick = false;
	stop = false;
}

void Parent_ball::collision()
{
	if (_position.x < GameLayout::BallLeftBoundary)
	{
		_position.x = GameLayout::BallLeftBoundary;
		_velocity = ReflectVelocity(_velocity, { 1.0f, 0.0f }, WALL_RESTITUTION);
	}
	else if (_position.x > GameLayout::BallRightBoundary)
	{
		_position.x = GameLayout::BallRightBoundary;
		_velocity = ReflectVelocity(_velocity, { -1.0f, 0.0f }, WALL_RESTITUTION);
	}
	if (_position.y < GameLayout::BallTopBoundary)
	{
		_position.y = GameLayout::BallTopBoundary;
		_velocity = ReflectVelocity(_velocity, { 0.0f, 1.0f }, WALL_RESTITUTION);
	}
}

void Parent_ball::movement(float timeScale)
{
	_velocity.y += _gravity * timeScale;
	_position += _velocity * (_force * timeScale);

}
