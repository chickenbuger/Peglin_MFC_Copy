#pragma once


class Parent_ball
{
public:
	Parent_ball();
	~Parent_ball() {}
private:
	float	pos[2]; //x,y
	float	_size;

	float	Fpos[2];
	float	Spos[2];
	float	CheckPos[2];
	
	float	_force;
	float	_force_x;
	float	_force_y;
	float	_gravity;
	float	_gravity_mul;

	float	_line_x;
	float	_line_y;

	float	_Current_Dir_x;
	float	_Current_Dir_y;

	bool	IsActive;
	bool	IsClick;

	bool	IsCrashToTargetball;
public:
	//setter
	void SetPos(float x, float y)		{ pos[0] = x; pos[1] = y; }
	void SetFPos(float x, float y)		{ Fpos[0] = x; Fpos[1] = y; }
	void SetSPos(float x, float y)		{ Spos[0] = x; Spos[1] = y; }
	void SetCheckPos(float x, float y)	{ CheckPos[0] = x; CheckPos[1] = y; }
	void SetSize(float size)			{ _size = size; }
	void SetGMUL(float mul)				{ _gravity_mul = mul; }
	void SetForce(float force)			{ _force = force; }
	void SetForceX(float forceX)		{ _force_x = forceX; }
	void SetForceY(float forceY)		{ _force_y = forceY; }
	void SetClick(bool click)			{ IsClick = click; }
	void SetCrash(bool Crash)			{ IsCrashToTargetball = Crash; }

	//getter
	float*	GetPos()		{ return pos; }
	float*	GetFPos()		{ return Fpos; }
	float*	GetSPos()		{ return Spos; }
	float*	GetCheckPos()	{ return CheckPos; }
	float	GetSize()		{ return _size; }
	float	GetForce()		{ return _force; }
	float	GetForceX()		{ return _force_x; }
	float	GetForceY()		{ return _force_y; }
	float	GetGravity()	{ return _gravity; }
	float	GetDirX()		{ return _Current_Dir_x; }
	float	GetDirY()		{ return _Current_Dir_y; }
	bool	GetActive()		{ return IsActive; }
	bool	GetClick()		{ return IsClick; }
	bool	GetCrash()		{ return IsCrashToTargetball; }
public:
	void shooting();
public:
	//º® °Ë»ç
	void draw(CDC* pDC);
	void update(float delta);
	void Init();
	void collision();
	
public:
	bool stop=false;
};