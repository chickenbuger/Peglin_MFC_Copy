#pragma once


class Parent_ball
{
public:
	Parent_ball();
	~Parent_ball() {}

public:
	//setter
	void SetStartDragPos(float x, float y)		{ StartDragPos[0] = x; StartDragPos[1] = y; }
	void SetEndDragPos(float x, float y)		{ EndDragPos[0] = x; EndDragPos[1] = y; }

	void SetTraceDragPos(float x, float y)		{ TraceDragPos[0] = x; TraceDragPos[1] = y; }
	void SetVelocityX(float vx)					{ _velocity_x = vx; }
	void SetVelocityY(float vy)					{ _velocity_y = vy; }
	void SetClick(bool click)					{ IsClick = click; }

	//getter
	float*	GetPos()				{ return pos; }

	float	GetSize()				{ return _size; }
	float	GetVelocityX()			{ return _velocity_x; }
	float	GetVelocityY()			{ return _velocity_y; }
	bool	GetActive()				{ return IsActive; }
	bool	GetClick()				{ return IsClick; }
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

	float	_gravity;

	bool	IsActive;
	bool	IsClick;

};
