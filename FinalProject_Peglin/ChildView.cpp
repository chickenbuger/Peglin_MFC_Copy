
// ChildView.cpp: CChildView 클래스의 구현
//

#include "pch.h"
#include "framework.h"
#include "FinalProject_Peglin.h"
#include "ChildView.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
{
	Init_ball();
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_32771, &CChildView::On32771)
END_MESSAGE_MAP()



// CChildView 메시지 처리기

void CChildView::gameclear()
{
	if (!check)
	{
		check = true;
		AfxMessageBox(_T("Game Clear!!"));
	}
	ResetGameState();
}

void CChildView::gameover()
{
	if (!check)
	{
		check = true;
		AfxMessageBox(_T("Game Over!!"));
	}
	ResetGameState();
}

void CChildView::Init_ball()
{
	_targetBallList._targetBallList.RemoveAll();
	for (int x = 0; x < (930 - 30) / 40; x++)
	{
		for (int y = 0; y < (650 - 400) / 60; y++)
		{
			TargetBall ball;
			ball.setting(50 + x * 80, 400 + y * 80);
			_targetBallList.add(ball);
		}
	}
}

void CChildView::Collision()
{
	auto pos = _targetBallList._targetBallList.GetHeadPosition();
	while (pos != nullptr)
	{
		auto cur = pos;
		auto& _target = _targetBallList._targetBallList.GetNext(pos);
		const float ball_x = _ball.GetPos()[0];
		const float ball_y = _ball.GetPos()[1];
		const float rx = ball_x - _target.x;
		const float ry = ball_y - _target.y;

		float distanceSquared = rx * rx + ry * ry;

		if (pow(rx, 2) + pow(ry, 2) <= pow((_ball.GetSize() + _target.size),2)) //충돌시
		{
			std::cout << "충돌\n";

			/*
			float distance = sqrt(distanceSquared);
			float normalX = rx / distance;
			float normalY = ry / distance;

			// 2️ 현재 속도 벡터 가져오기
			float velocityX = _ball.GetVelocityX();
			float velocityY = _ball.GetVelocityY();

			// 3️ 벡터 반사 공식 적용: V' = V - 2 (V · N) N
			float dotProduct = (velocityX * normalX) + (velocityY * normalY);
			float reflectX = velocityX - 2 * dotProduct * normalX;
			float reflectY = velocityY - 2 * dotProduct * normalY;

			// 4️ 감속 적용 (충돌 후 점차 속도가 줄어들도록)
			float reboundFactor = 0.8f;  // 반사 후 속도 감소율
			_ball.SetForceX(reflectX * reboundFactor);
			_ball.SetForceY(reflectY * reboundFactor);

			*/

			// 5️ targetBall 제거
			_targetBallList._targetBallList.RemoveAt(cur);
			ball_DMG += 1.0f;
		}
	}
}

void CChildView::restart()
{
	ResetGameState();
}

void CChildView::ResetGameState()
{
	check = false;
	_ball.Init();
	_player.Init();
	_enemy.Init();
	Init_ball();
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

void CChildView::OnPaint() 
{
	CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.
	
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	CRect rect;
	GetClientRect(&rect);

	CDC memDc;
	memDc.CreateCompatibleDC(&dc);
	CBitmap  bitmap;
	bitmap.CreateCompatibleBitmap(&dc, rect.right, rect.bottom);
	memDc.SelectObject(&bitmap);
	
	_background.draw(&memDc);

	//player
	_player.draw(&memDc);

	//enemy
	_enemy.draw(&memDc);

	//ball
	_ball.draw(&memDc);
	
	//targetball
	auto pos = _targetBallList._targetBallList.GetHeadPosition();
	while (pos != nullptr)
	{
		auto& _target = _targetBallList._targetBallList.GetNext(pos);
		_target.draw(&memDc);
	}

	if (_player.GetHp() > 0.0f)
	{
		CString Text;
		Text.Format(_T("플레이어 체력 : %d "), static_cast<int>(_player.GetHp()));
		memDc.TextOut(100, 70, Text);
	}
	if (_enemy.GetHp() > 0.0f)
	{
		CString Text1;
		Text1.Format(_T("몬스터 체력 : %d"), static_cast<int>(_enemy.GetHp()));
		memDc.TextOut(_enemy.GetX() - 30.0f, 90, Text1);
	}

	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
	// 그리기 메시지에 대해서는 CWnd::OnPaint()를 호출하지 마십시오.
}



int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  여기에 특수화된 작성 코드를 추가합니다.
	SetTimer(0, 10, NULL);
	SetTimer(1, 10, NULL);

	return 0;
}


void CChildView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == 0)
	{
		//Collision();
	}
	if (nIDEvent == 1)
	{
		_ball.update();
		
		if (_ball.GetPos()[1] > 800.0f)
		{
			if (_enemy.GetCount() < 8)
			{
				_enemy.SetX(_enemy.GetX() - 64);
			}
			else
			{
				_player.SetHp(_player.GetHp() - 20.0f);
			}
			_enemy.SetCount(_enemy.GetCount() + 1);
			_enemy.SetHp(_enemy.GetHp() - ball_DMG);
			ball_DMG = 0.0f;
			_ball.Init();
		}
		
		if (_player.GetHp() <= 0)
		{
			gameover();
		}
		else if (_enemy.GetHp() <= 0)
		{
			gameclear();
		}
		
	}
	Invalidate();

	CWnd::OnTimer(nIDEvent);
}


void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (_ball.GetActive() == false)
	{
		_ball.SetStartDragPos(point.x, point.y);
		_ball.SetClick(true);
	}
	CWnd::OnLButtonDown(nFlags, point);
}


void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (_ball.GetActive() == false)
	{
		_ball.SetEndDragPos(point.x, point.y);
		_ball.SetClick(false);
		_ball.shooting();
	}
	CWnd::OnLButtonUp(nFlags, point);
}


BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	return true;
}


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (_ball.GetClick() == true)
	{
		_ball.SetTraceDragPos(point.x, point.y);
		Invalidate();
	}

	CWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nChar == VK_SPACE)
	{
		_ball.stop = !_ball.stop;

		Invalidate();
	}
	if (nChar == VK_F5)
	{
		_ball.Init();
		Invalidate();
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CChildView::On32771()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	restart();
}
