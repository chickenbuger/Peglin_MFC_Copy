
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
	virtual ~CChildView();

// 특성입니다.
public:
	Player			_player;
	Enemy			_enemy;
	Parent_ball		_ball;
	Background		_background;
	TargetBallList	_targetBallList;

	float ball_DMG = 0.0f;

	bool check = false;

public:
	void gameclear();
	void gameover();
	void restart();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

protected:
	afx_msg int	OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void On32771();

	void RestrictMouseToWindow();
	void KillMouse();

private:
	void ResetGameState();
	void Init_ball();
	void Collision();
};

