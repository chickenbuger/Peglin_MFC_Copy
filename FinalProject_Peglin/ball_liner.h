#pragma once
class ball_liner
{
public:
	ball_liner() {}
	~ball_liner() {}
private:
	float	 pos[2];
	float	_size;

	float	Fpos[2];
	float	CheckPos[2];

	float	_force;
	float	_gravity;
	float	_gravity_mul = 0.06f;

	float	_lifetime = 5.0f;

	bool	_IsActive = false;
public:
	void SetPos(float x, float y)		{ pos[0] = x; pos[1] = y; }
	void SetFPos(float x, float y)		{ Fpos[0] = x; Fpos[1] = y; }
	void SetCheckPos(float x, float y)	{ CheckPos[0] = x; CheckPos[1] = y; }
	void SetSize(float size)			{ _size = size; }
	void SetForce(float force)			{ _force = force; }
	void SetLife(float life)			{ _lifetime = life; }
	void SetActive(float active)		{ _IsActive = active; }
	void Setgravity(float gravity)		{ _gravity = gravity; }
	void SetgravityMUL(float Mul)		{ _gravity_mul = Mul; }

	float*	GetPos()		{ return pos; }
	float*	GetFPos()		{ return Fpos; }
	float*	GetCheckPos()	{ return CheckPos; }
	float	GetSize()		{ return _size; }
	float	GetForce()		{ return _force; }
	float	GetLife()		{ return _lifetime; }

public:
	void draw(CDC* pDC);
	void update(float delta);
	bool isDead();
};

class LinerHandler
{
public:
	CList<ball_liner> _ballList;
public:
	LinerHandler(void) {}
	~LinerHandler(void) {}
public:
	inline void add(ball_liner& b) { _ballList.AddTail(b); }
};