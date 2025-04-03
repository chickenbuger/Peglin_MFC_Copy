
// ChildView.h: CChildView 클래스의 인터페이스
//


#pragma once

#include "Player.h"
#include "Enemy.h"
#include "Parent_ball.h"
#include "Background.h"
#include "TargetBall.h"

// CChildView 창

class CChildView : public CWnd
{
// 생성입니다.
public:
	CChildView();

// 특성입니다.
public:
	Player			_player;
	Enemy			_enemy;
	Parent_ball		_ball;
	Background		_background;
	TargetBallList	_targetBallList;

	float ball_DMG = 0.0f;

	bool check = false;

	float Dir_x = 1;
	float Dir_y = 1;
	float test1 = 0;
	float test = 0;

public:
	void gameclear();
	void gameover();
	void restart();

private:
	void ResetGameState();
	void Init_ball();
	void Collision();

// 재정의입니다.
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// 구현입니다.
public:
	virtual ~CChildView();

	// 생성된 메시지 맵 함수
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void On32771();
};

