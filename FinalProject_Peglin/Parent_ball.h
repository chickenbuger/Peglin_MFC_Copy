#pragma once


#include "Vector2.h"

class Parent_ball
{
public:
	Parent_ball();
	~Parent_ball() {}

public:
	//setter
	void SetStartDragPos(Vector2 position)		{ _startDragPosition = position; }
	void SetEndDragPos(Vector2 position)		{ _endDragPosition = position; }
	void SetTraceDragPos(Vector2 position)		{ _traceDragPosition = position; }
	void SetPosition(Vector2 position)			{ _position = position; }
	void SetVelocity(Vector2 velocity)			{ _velocity = velocity; }
	void SetClick(bool click)					{ IsClick = click; }

	//getter
	const Vector2& GetPosition() const noexcept	{ return _position; }
	const Vector2& GetVelocity() const noexcept	{ return _velocity; }
	float GetSize() const noexcept					{ return _size; }
	bool GetActive() const noexcept					{ return IsActive; }
	bool GetClick() const noexcept					{ return IsClick; }
public:
	bool shooting();
	void collision();

public:
	//벽 검사
	void draw(CDC* pDC);
	void update(float deltaSeconds);
	void Init();
	
public:
	bool stop=false;

private:
	void drawline(CDC* pDC);
	void movement(float timeScale);

private:
	Vector2 _position;
	float _size = 0.0f;

	//Drag 시작 위치
	Vector2 _startDragPosition;
	//Drag 중 라인 위치
	Vector2 _traceDragPosition;
	//Drag 마지막 위치
	Vector2 _endDragPosition;

	float	_force = 0.0f;
	Vector2 _velocity;

	float	_gravity;

	bool	IsActive;
	bool	IsClick;

};
