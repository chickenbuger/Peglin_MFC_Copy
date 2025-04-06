#pragma once


class Parent_ball
{
public:
	Parent_ball();
	~Parent_ball() {}

public:
	//setter
	void SetPos(float x, float y)				{ pos[0] = x; pos[1] = y; }
	void SetStartDragPos(float x, float y)		{ StartDragPos[0] = x; StartDragPos[1] = y; }
	void SetEndDragPos(float x, float y)		{ EndDragPos[0] = x; EndDragPos[1] = y; }

	void SetTraceDragPos(float x, float y)		{ TraceDragPos[0] = x; TraceDragPos[1] = y; }
	void SetSize(float size)					{ _size = size; }
	void SetForce(float force)					{ _force = force; }
	void SetVelocityX(float vx)					{ _velocity_x = vx; }
	void SetVelocityY(float vy)					{ _velocity_y = vy; }
	void SetClick(bool click)					{ IsClick = click; }
	void SetCrash(bool Crash)					{ IsCrashToTargetball = Crash; }

	//getter
	float*	GetPos()				{ return pos; }

	float*	GetStartDragPos()		{ return StartDragPos; }
	float*	GetTraceDragPos()		{ return TraceDragPos; }
	float*	GetEndDragPos()			{ return EndDragPos; }

	float	GetSize()				{ return _size; }
	float	GetForce()				{ return _force; }
	float	GetVelocityX()			{ return _velocity_x; }
	float	GetVelocityY()			{ return _velocity_y; }
	float	GetGravity()			{ return _gravity; }
	bool	GetActive()				{ return IsActive; }
	bool	GetClick()				{ return IsClick; }
	bool	GetCrash()				{ return IsCrashToTargetball; }
public:
	void shooting();
	void collision();

public:
	//벽 검사
	void draw(CDC* pDC);
	void update();
	void Init();
	
public:
	bool stop=false;

private:
	void drawline(CDC* pDC);
	void movement();

private:
	float	pos[2] = { 490.f,250.f };
	float	_size = { 10.f };

	//Drag 시작 위치
	float	StartDragPos[2];
	//Drag 중 라인 위치
	float	TraceDragPos[2];
	//Drag 마지막 위치
	float	EndDragPos[2];

	float	_force = 0.0f;
	float	_velocity_x = 0.0f;
	float	_velocity_y = 0.0f;

	float	_mass = 1.0f;

	float	_gravity;

	bool	IsActive;
	bool	IsClick;

	bool	IsCrashToTargetball;
};