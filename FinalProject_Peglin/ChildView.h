
// ChildView.h: CChildView 클래스의 인터페이스
//


#pragma once

#include <chrono>

#include "Player.h"
#include "Enemy.h"
#include "Parent_ball.h"
#include "Background.h"
#include "TargetBall.h"
#include "GameState.h"

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

public:
	void gameclear();
	void gameover();
	void restart();
	GameState GetGameState() const noexcept { return _gameState; }

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

protected:
	afx_msg int	OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCaptureChanged(CWnd* pWnd);
	afx_msg void On32771();

private:
	void ReleaseMouseInput(bool cancelDrag);
	void ResetGameState();
	void Init_ball();
	void Collision();
	void UpdateGameStep(float deltaSeconds);
	void ResolveTurn();
	bool TransitionTo(GameState nextState);

	UINT_PTR _gameTimerId = 0;
	std::chrono::steady_clock::time_point _lastFrameTime{};
	double _accumulatedTimeSeconds = 0.0;
	float _pendingDamage = 0.0f;
	GameState _gameState = GameState::Aiming;
	GameState _stateBeforePause = GameState::Aiming;
};

