
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

		if (distanceSquared <= pow((_ball.GetSize() + _target.size),2)) //충돌시
		{
			std::cout << "충돌\n";

			float distance = sqrt(distanceSquared);
			if (distance == 0.0f) distance = 0.01f; // 0으로 나누는 오류 방지

			//충돌


			// 1. 법선 벡터 구하기 (단위 벡터)
			float nx = rx / distance;
			float ny = ry / distance;

			// 2. 수직 벡터 구하기
			float tx = -ny;
			float ty = nx;

			// 3. A(메인 볼) 속도 가져오기
			float ball_vx = _ball.GetVelocityX();
			float ball_vy = _ball.GetVelocityY();

			// 4. B(타겟 볼) 속도 가져오기 (지금은 가만히 있다고 했지만, 일반화를 위해 추가)
			float target_vx = 0.0f;
			float target_vy = 0.0f;

			// 5. 속도를 법선/수직 방향으로 분해
			float ball_vn = ball_vx * nx + ball_vy * ny;
			float ball_vt = ball_vx * tx + ball_vy * ty;

			float target_vn = target_vx * nx + target_vy * ny;
			float target_vt = target_vx * tx + target_vy * ty;

			// 6. 질량이 같으면 법선 성분만 서로 교환
			float ball_vn_after = target_vn;
			float target_vn_after = ball_vn;

			// 7. 분해된 성분을 다시 합쳐서 실제 x, y 속도로 복원
			_ball.SetVelocityX(ball_vn_after * nx + ball_vt * tx);
			_ball.SetVelocityY(ball_vn_after * ny + ball_vt * ty);


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
		Collision();
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
	CRect clientRect;
	GetClientRect(&clientRect);

	ClientToScreen(&clientRect); // 클라이언트 좌표를 화면 좌표로 변환

	POINT cursorPos;
	GetCursorPos(&cursorPos); // 현재 마우스 위치 가져오기

	if (cursorPos.x < clientRect.left || cursorPos.x > clientRect.right || cursorPos.y < clientRect.top || cursorPos.y > clientRect.bottom)
	{
		RestrictMouseToWindow(); // 창 내부로 제한
	}

	//드래그 처리
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

void CChildView::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	RestrictMouseToWindow();
}

void CChildView::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	KillMouse();
}


void CChildView::On32771()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	restart();
}

void CChildView::RestrictMouseToWindow()
{
	CRect rect;
	GetWindowRect(&rect);
	ClipCursor(&rect);
}

void CChildView::KillMouse()
{
	ClipCursor(NULL);
}
