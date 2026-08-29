
// ChildView.cpp: CChildView 클래스의 구현
//

#include "pch.h"
#include "framework.h"
#include "FinalProject_Peglin.h"
#include "ChildView.h"
#include "GameLayout.h"
#include <algorithm>
#include <cmath>
#include <utility>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	CString StateText(GameState state)
	{
		switch (state)
		{
		case GameState::Aiming: return _T("상태: 조준 중");
		case GameState::BallInFlight: return _T("상태: 공 비행 중");
		case GameState::ResolvingTurn: return _T("상태: 턴 정산 중");
		case GameState::Victory: return _T("상태: 승리");
		case GameState::Defeat: return _T("상태: 패배");
		case GameState::Paused: return _T("상태: 일시정지");
		}

		return _T("상태: 알 수 없음");
	}

	CString FeedbackText(const GameFeedback& feedback, const GameScore& score)
	{
		CString text;
		switch (feedback.type)
		{
		case GameFeedbackType::Ready:
			text.Format(_T("발사 준비 · 총 %d · 최고 콤보 %d"), score.total, score.bestCombo);
			break;
		case GameFeedbackType::ShotLaunched:
		case GameFeedbackType::PegHit:
		case GameFeedbackType::Paused:
			text.Format(
				_T("적중 %d · 콤보 %d · 발사 %d"),
				feedback.currentShotPegHits,
				score.currentCombo,
				score.currentShot);
			break;
		case GameFeedbackType::TurnResolved:
			text.Format(
				_T("턴 %d · 피해 %d · +%d · 총 %d"),
				feedback.turnNumber,
				static_cast<int>(feedback.lastEnemyDamage),
				score.lastTurn,
				score.total);
			break;
		case GameFeedbackType::PlayerDamaged:
			text.Format(
				_T("턴 %d · 피격 %d · +%d · 총 %d"),
				feedback.turnNumber,
				static_cast<int>(feedback.lastPlayerDamage),
				score.lastTurn,
				score.total);
			break;
		case GameFeedbackType::Victory:
			text.Format(_T("몬스터를 처치했습니다 · 최종 점수 %d"), score.total);
			break;
		case GameFeedbackType::Defeat:
			text.Format(_T("플레이어가 쓰러졌습니다 · 최종 점수 %d"), score.total);
			break;
		}

		return text;
	}

	COLORREF PegEffectColor(PegType type)
	{
		switch (type)
		{
		case PegType::Normal: return RGB(255, 80, 80);
		case PegType::Critical: return RGB(255, 190, 0);
		case PegType::Bomb: return RGB(255, 120, 0);
		case PegType::Refresh: return RGB(0, 190, 90);
		}

		return RGB(255, 255, 255);
	}
}

// CChildView

CChildView::CChildView()
{
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CAPTURECHANGED()
	ON_COMMAND(ID_32771, &CChildView::On32771)
END_MESSAGE_MAP()



// CChildView 메시지 처리기

void CChildView::gameclear()
{
	AfxMessageBox(_T("Game Clear!!"));
	_game.ResetGame();
	_feedbackAnimations.clear();
}

void CChildView::gameover()
{
	AfxMessageBox(_T("Game Over!!"));
	_game.ResetGame();
	_feedbackAnimations.clear();
}

void CChildView::restart()
{
	_game.ResetGame();
	_feedbackAnimations.clear();
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
	CBitmap* previousBitmap = memDc.SelectObject(&bitmap);
	
	_background.draw(&memDc);

	//player
	_game.GetPlayer().draw(&memDc);

	//enemy
	_game.GetEnemy().draw(&memDc);

	//ball
	_game.GetBall().draw(&memDc);
	
	//targetball
	auto& targets = _game.GetTargets()._targetBallList;
	auto pos = targets.GetHeadPosition();
	while (pos != nullptr)
	{
		auto& _target = targets.GetNext(pos);
		_target.draw(&memDc);
	}

	DrawAimPreview(&memDc);

	if (_game.GetPlayer().GetHp() > 0.0f)
	{
		CString Text;
		Text.Format(_T("플레이어 체력 : %d "), static_cast<int>(_game.GetPlayer().GetHp()));
		memDc.TextOut(
			static_cast<int>(std::lround(GameLayout::PlayerHealthText.x)),
			static_cast<int>(std::lround(GameLayout::PlayerHealthText.y)),
			Text);
	}
	if (_game.GetEnemy().GetHp() > 0.0f)
	{
		CString Text1;
		Text1.Format(_T("몬스터 체력 : %d"), static_cast<int>(_game.GetEnemy().GetHp()));
		memDc.TextOut(
			static_cast<int>(std::lround(_game.GetEnemy().GetX() + GameLayout::EnemyHealthTextOffsetX)),
			static_cast<int>(std::lround(GameLayout::EnemyHealthTextY)),
			Text1);
	}

	const int textState = memDc.SaveDC();
	memDc.SetBkMode(TRANSPARENT);
	memDc.SetTextColor(RGB(0, 0, 0));
	memDc.TextOut(
		static_cast<int>(std::lround(GameLayout::StateText.x)),
		static_cast<int>(std::lround(GameLayout::StateText.y)),
		StateText(_game.GetState()));
	memDc.TextOut(
		static_cast<int>(std::lround(GameLayout::FeedbackText.x)),
		static_cast<int>(std::lround(GameLayout::FeedbackText.y)),
		FeedbackText(_game.GetFeedback(), _game.GetScore()));
	memDc.TextOut(
		static_cast<int>(std::lround(GameLayout::OptionsText.x)),
		static_cast<int>(std::lround(GameLayout::OptionsText.y)),
		_soundEnabled ? _T("사운드: 켜짐 (M)") : _T("사운드: 꺼짐 (M)"));
	memDc.RestoreDC(textState);

	DrawFeedbackAnimations(&memDc);

	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
	memDc.SelectObject(previousBitmap);
	// 그리기 메시지에 대해서는 CWnd::OnPaint()를 호출하지 마십시오.
}



int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	constexpr UINT GAME_TIMER_INTERVAL_MS = 10;
	_gameTimerId = SetTimer(1, GAME_TIMER_INTERVAL_MS, nullptr);
	if (_gameTimerId == 0)
	{
		return -1;
	}

	_lastFrameTime = std::chrono::steady_clock::now();
	_accumulatedTimeSeconds = 0.0;

	return 0;
}

void CChildView::OnDestroy()
{
	ReleaseMouseInput(true);

	if (_gameTimerId != 0)
	{
		KillTimer(_gameTimerId);
		_gameTimerId = 0;
	}

	CWnd::OnDestroy();
}

void CChildView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != _gameTimerId || _gameTimerId == 0)
	{
		CWnd::OnTimer(nIDEvent);
		return;
	}

	constexpr double FIXED_TIMESTEP_SECONDS = 0.01;
	constexpr double MAX_FRAME_TIME_SECONDS = 0.1;

	const auto now = std::chrono::steady_clock::now();
	const double elapsedSeconds = std::chrono::duration<double>(now - _lastFrameTime).count();
	_lastFrameTime = now;
	_accumulatedTimeSeconds += std::clamp(elapsedSeconds, 0.0, MAX_FRAME_TIME_SECONDS);

	while (_accumulatedTimeSeconds >= FIXED_TIMESTEP_SECONDS)
	{
		UpdateGameStep(static_cast<float>(FIXED_TIMESTEP_SECONDS));
		_accumulatedTimeSeconds -= FIXED_TIMESTEP_SECONDS;
	}

	Invalidate(FALSE);

	CWnd::OnTimer(nIDEvent);
}

void CChildView::UpdateGameStep(float deltaSeconds)
{
	const GameUpdateResult result = _game.Update(deltaSeconds);
	ConsumeGameEvents();
	UpdateFeedbackAnimations(deltaSeconds);

	switch (result)
	{
	case GameUpdateResult::None:
		break;
	case GameUpdateResult::Victory:
		gameclear();
		break;
	case GameUpdateResult::Defeat:
		gameover();
		break;
	}
}

void CChildView::ConsumeGameEvents()
{
	for (const GameEvent& event : _game.ConsumeEvents())
	{
		FeedbackAnimation animation;
		animation.position = event.position;
		animation.color = PegEffectColor(event.pegType);

		switch (event.type)
		{
		case GameEventType::PegHit:
			if (event.pegType == PegType::Critical)
			{
				animation.text.Format(_T("CRIT +%d"), event.scoreAwarded);
			}
			else
			{
				animation.text.Format(_T("+%d · x%d"), event.scoreAwarded, event.combo);
			}
			break;
		case GameEventType::BombTriggered:
			animation.text.Format(_T("BOOM! %d"), event.affectedPegs);
			animation.color = RGB(255, 120, 0);
			animation.lifetimeSeconds = 1.1f;
			break;
		case GameEventType::RefreshTriggered:
			animation.text.Format(_T("REFRESH +%d"), event.affectedPegs);
			animation.color = RGB(0, 190, 90);
			animation.lifetimeSeconds = 1.1f;
			break;
		case GameEventType::TurnResolved:
			animation.text.Format(_T("TURN +%d"), event.scoreAwarded);
			animation.position = GameLayout::TurnEffectPosition;
			animation.color = RGB(40, 100, 220);
			break;
		case GameEventType::PlayerDamaged:
			animation.text.Format(_T("HP -%d"), static_cast<int>(event.damage));
			animation.color = RGB(220, 0, 0);
			animation.lifetimeSeconds = 1.2f;
			break;
		case GameEventType::Victory:
			animation.text = _T("CLEAR!");
			animation.position = GameLayout::TurnEffectPosition;
			animation.color = RGB(0, 150, 60);
			break;
		case GameEventType::Defeat:
			animation.text = _T("GAME OVER");
			animation.position = GameLayout::TurnEffectPosition;
			animation.color = RGB(220, 0, 0);
			break;
		}

		_feedbackAnimations.push_back(std::move(animation));
		PlayEventSound(event.type, event.pegType);
	}
}

void CChildView::UpdateFeedbackAnimations(float deltaSeconds)
{
	for (FeedbackAnimation& animation : _feedbackAnimations)
	{
		animation.ageSeconds += deltaSeconds;
	}

	_feedbackAnimations.erase(
		std::remove_if(
			_feedbackAnimations.begin(),
			_feedbackAnimations.end(),
			[](const FeedbackAnimation& animation)
			{
				return animation.ageSeconds >= animation.lifetimeSeconds;
			}),
		_feedbackAnimations.end());
}

void CChildView::DrawFeedbackAnimations(CDC* deviceContext)
{
	const int savedDc = deviceContext->SaveDC();
	deviceContext->SetBkMode(TRANSPARENT);

	CFont effectFont;
	effectFont.CreatePointFont(140, _T("맑은 고딕"));
	deviceContext->SelectObject(&effectFont);

	for (const FeedbackAnimation& animation : _feedbackAnimations)
	{
		const float progress = animation.ageSeconds / animation.lifetimeSeconds;
		deviceContext->SetTextColor(animation.color);
		deviceContext->TextOut(
			static_cast<int>(std::lround(animation.position.x - 30.0f)),
			static_cast<int>(std::lround(animation.position.y - progress * 45.0f)),
			animation.text);
	}

	deviceContext->RestoreDC(savedDc);
}

void CChildView::DrawAimPreview(CDC* deviceContext)
{
	const AimPreview preview = _game.GetAimPreview();
	if (!preview.visible)
	{
		return;
	}

	const int savedDc = deviceContext->SaveDC();
	const BYTE red = static_cast<BYTE>(std::lround(255.0f * preview.normalizedStrength));
	const BYTE green = static_cast<BYTE>(std::lround(220.0f * (1.0f - preview.normalizedStrength)));
	const COLORREF strengthColor = RGB(red, green, 60);

	CPen pathPen(PS_DOT, 2, RGB(210, 210, 255));
	deviceContext->SelectObject(&pathPen);
	deviceContext->SelectObject(GetStockObject(NULL_BRUSH));
	deviceContext->MoveTo(
		static_cast<int>(std::lround(_game.GetBall().GetPosition().x)),
		static_cast<int>(std::lround(_game.GetBall().GetPosition().y)));
	for (const Vector2 point : preview.points)
	{
		deviceContext->LineTo(
			static_cast<int>(std::lround(point.x)),
			static_cast<int>(std::lround(point.y)));
	}

	const int left = static_cast<int>(std::lround(GameLayout::AimStrengthPosition.x));
	const int top = static_cast<int>(std::lround(GameLayout::AimStrengthPosition.y));
	const int right = static_cast<int>(std::lround(left + GameLayout::AimStrengthWidth));
	const int bottom = static_cast<int>(std::lround(top + GameLayout::AimStrengthHeight));
	CBrush strengthBrush(strengthColor);
	deviceContext->SelectObject(&strengthBrush);
	deviceContext->SelectObject(GetStockObject(NULL_PEN));
	deviceContext->Rectangle(
		left,
		top,
		static_cast<int>(std::lround(left + GameLayout::AimStrengthWidth * preview.normalizedStrength)),
		bottom);
	deviceContext->SelectObject(GetStockObject(NULL_BRUSH));
	deviceContext->SelectObject(GetStockObject(WHITE_PEN));
	deviceContext->Rectangle(left, top, right, bottom);

	CString strengthText;
	strengthText.Format(
		_T("POWER %d%%"),
		static_cast<int>(std::lround(preview.normalizedStrength * 100.0f)));
	deviceContext->SetBkMode(TRANSPARENT);
	deviceContext->SetTextColor(RGB(255, 255, 255));
	deviceContext->TextOut(left, bottom + 3, strengthText);
	deviceContext->RestoreDC(savedDc);
}

void CChildView::PlayEventSound(GameEventType eventType, PegType pegType)
{
	if (!_soundEnabled)
	{
		return;
	}

	UINT sound = MB_OK;
	if (eventType == GameEventType::PlayerDamaged || eventType == GameEventType::Defeat)
	{
		sound = MB_ICONHAND;
	}
	else if (eventType == GameEventType::Victory || pegType == PegType::Refresh)
	{
		sound = MB_ICONASTERISK;
	}
	else if (eventType == GameEventType::BombTriggered || pegType == PegType::Critical)
	{
		sound = MB_ICONEXCLAMATION;
	}

	::MessageBeep(sound);
}


void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (_game.BeginAim({ static_cast<float>(point.x), static_cast<float>(point.y) }))
	{
		SetFocus();
		SetCapture();
	}
	CWnd::OnLButtonDown(nFlags, point);
}


void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	_game.ReleaseShot({ static_cast<float>(point.x), static_cast<float>(point.y) });
	ReleaseMouseInput(false);
	CWnd::OnLButtonUp(nFlags, point);
}


BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	UNREFERENCED_PARAMETER(pDC);

	return TRUE;
}


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	//드래그 처리
	if (_game.GetBall().GetClick())
	{
		_game.UpdateAim({ static_cast<float>(point.x), static_cast<float>(point.y) });
		Invalidate();
	}

	CWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nChar == VK_SPACE)
	{
		_game.TogglePause();

		Invalidate();
	}
	if (nChar == VK_F5)
	{
		_game.ResetBallToAiming();
		Invalidate();
	}
	if (nChar == 'M')
	{
		_soundEnabled = !_soundEnabled;
		Invalidate();
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CChildView::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
}

void CChildView::OnKillFocus(CWnd* pNewWnd)
{
	ReleaseMouseInput(true);
	CWnd::OnKillFocus(pNewWnd);
}

void CChildView::OnCaptureChanged(CWnd* pWnd)
{
	if (GetCapture() != this)
	{
		_game.CancelAim();
		::ClipCursor(nullptr);
	}

	CWnd::OnCaptureChanged(pWnd);
}

void CChildView::On32771()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	restart();
}

void CChildView::ReleaseMouseInput(bool cancelDrag)
{
	if (cancelDrag)
	{
		_game.CancelAim();
	}

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}

	::ClipCursor(nullptr);
}
