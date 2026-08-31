
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

#pragma comment(lib, "Msimg32.lib")

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

	COLORREF PegEffectColor(PegType type, PegColorMode colorMode)
	{
		if (colorMode == PegColorMode::HighContrast)
		{
			switch (type)
			{
			case PegType::Normal: return RGB(30, 130, 255);
			case PegType::Critical: return RGB(255, 220, 0);
			case PegType::Bomb: return RGB(245, 245, 245);
			case PegType::Refresh: return RGB(0, 255, 180);
			}
		}

		switch (type)
		{
		case PegType::Normal: return RGB(255, 80, 80);
		case PegType::Critical: return RGB(255, 190, 0);
		case PegType::Bomb: return RGB(255, 120, 0);
		case PegType::Refresh: return RGB(0, 190, 90);
		}

		return RGB(255, 255, 255);
	}

	CString EnemyActionText(const EnemyActionDefinition& action)
	{
		CString text;
		switch (action.type)
		{
		case EnemyActionType::Advance:
			text.Format(_T("다음: 이동 %d"), static_cast<int>(std::lround(action.magnitude)));
			break;
		case EnemyActionType::Strike:
			text.Format(_T("다음: 공격 %d"), static_cast<int>(std::lround(action.magnitude)));
			break;
		case EnemyActionType::Fortify:
			text.Format(_T("다음: 방어막 %d"), static_cast<int>(std::lround(action.magnitude)));
			break;
		}
		return text;
	}

	CString DifficultyText(GameDifficulty difficulty)
	{
		switch (difficulty)
		{
		case GameDifficulty::Easy: return _T("쉬움");
		case GameDifficulty::Normal: return _T("보통");
		case GameDifficulty::Hard: return _T("어려움");
		}

		return _T("보통");
	}

	CString PegColorModeText(PegColorMode colorMode)
	{
		return colorMode == PegColorMode::HighContrast
			? _T("고대비 + 모양")
			: _T("표준 색상");
	}

	CString Utf8Text(std::string_view text)
	{
		if (text.empty())
		{
			return {};
		}
		const int characterCount = MultiByteToWideChar(
			CP_UTF8,
			MB_ERR_INVALID_CHARS,
			text.data(),
			static_cast<int>(text.size()),
			nullptr,
			0);
		if (characterCount <= 0)
		{
			return _T("Invalid text");
		}
		CString converted;
		wchar_t* buffer = converted.GetBuffer(characterCount);
		MultiByteToWideChar(
			CP_UTF8,
			MB_ERR_INVALID_CHARS,
			text.data(),
			static_cast<int>(text.size()),
			buffer,
			characterCount);
		converted.ReleaseBuffer(characterCount);
		return converted;
	}

	CString RelicSummary(const PlayerLoadout& loadout)
	{
		CString summary;
		for (const RelicDefinition& relic : GetRelicDefinitions())
		{
			const std::size_t stacks = loadout.GetRelicStackCount(relic.id);
			if (stacks == 0)
			{
				continue;
			}
			if (!summary.IsEmpty())
			{
				summary += _T(" · ");
			}
			summary += Utf8Text(relic.displayName);
			if (stacks > 1)
			{
				CString stackText;
				stackText.Format(_T(" x%zu"), stacks);
				summary += stackText;
			}
		}
		if (summary.IsEmpty())
		{
			return CString(_T("유물 없음"));
		}
		return summary;
	}

	COLORREF OrbAccent(std::string_view orbId)
	{
		if (orbId == "iron-orb")
		{
			return RGB(185, 194, 204);
		}
		if (orbId == "echo-orb")
		{
			return RGB(180, 110, 235);
		}
		return RGB(45, 210, 190);
	}

	void DrawOrbIcon(CDC* deviceContext, const CRect& bounds, std::string_view orbId)
	{
		const int savedDc = deviceContext->SaveDC();
		const COLORREF accent = OrbAccent(orbId);
		CPen outerPen(PS_SOLID, 2, UiTheme::Gold);
		CBrush orbBrush(accent);
		deviceContext->SelectObject(&outerPen);
		deviceContext->SelectObject(&orbBrush);
		deviceContext->Ellipse(bounds);
		CRect highlight(bounds);
		highlight.DeflateRect(bounds.Width() / 4, bounds.Height() / 4);
		CBrush highlightBrush(RGB(245, 238, 190));
		deviceContext->SelectObject(&highlightBrush);
		deviceContext->SelectStockObject(NULL_PEN);
		deviceContext->Ellipse(highlight);
		deviceContext->RestoreDC(savedDc);
	}

	std::filesystem::path GetDefaultContentCatalogPath()
	{
		std::vector<wchar_t> modulePath(1024, L'\0');
		const DWORD length = GetModuleFileNameW(
			nullptr,
			modulePath.data(),
			static_cast<DWORD>(modulePath.size()));
		if (length == 0 || length >= modulePath.size())
		{
			return std::filesystem::path(L"content") / L"stages.v1.ini";
		}
		return std::filesystem::path(modulePath.data()).parent_path()
			/ L"content" / L"stages.v1.ini";
	}
}

// CChildView

CChildView::CChildView()
	: _settingsStore(GetDefaultGameSettingsPath()),
	_recordStore(GetDefaultGameRecordPath())
{
	const SettingsLoadResult settings = _settingsStore.Load();
	_options = settings.options;
	if (settings.state == SettingsLoadState::Migrated)
	{
		_settingsSaveFailed = !_settingsStore.Save(_options);
	}
	const RecordLoadResult records = _recordStore.Load();
	_records = records.records;
	if (records.state == RecordLoadState::Migrated)
	{
		_recordSaveFailed = !_recordStore.Save(_records);
	}
	_contentCatalog = LoadContentCatalog(GetDefaultContentCatalogPath());
	BeginNewRun();
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
	_resultSummary = _game.GetResultSummary();
	RecordResult(true);
	_runPlayerHealth = _game.GetPlayer().GetHp();
	_run.CompleteCurrentStage();
	_screenMode = _run.GetStatus() == RunStatus::RewardSelection
		? ScreenMode::Reward
		: ScreenMode::Result;
	ReleaseMouseInput(true);
	_feedbackAnimations.clear();
	_orbTrail.clear();
}

void CChildView::gameover()
{
	_resultSummary = _game.GetResultSummary();
	RecordResult(false);
	_run.MarkDefeated();
	_screenMode = ScreenMode::Result;
	ReleaseMouseInput(true);
	_feedbackAnimations.clear();
	_orbTrail.clear();
}

void CChildView::restart()
{
	_game.ResetGame();
	_feedbackAnimations.clear();
	_orbTrail.clear();
	_orbTrailSampleSeconds = 0.0f;
	_gameplayVisualTimeSeconds = 0.0f;
	_resultSummary.reset();
	_screenMode = ScreenMode::Playing;
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
	
	_background.draw(
		&memDc,
		_gameplayBackground.GetSafeHandle() != nullptr ? &_gameplayBackground : nullptr);

	if (_screenMode == ScreenMode::StageSelection)
	{
		DrawStageSelection(&memDc);
		dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
		memDc.SelectObject(previousBitmap);
		return;
	}
	if (_screenMode == ScreenMode::Loadout)
	{
		DrawLoadoutScreen(&memDc);
		dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
		memDc.SelectObject(previousBitmap);
		return;
	}
	if (_screenMode == ScreenMode::Options)
	{
		DrawOptions(&memDc);
		dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
		memDc.SelectObject(previousBitmap);
		return;
	}
	if (_screenMode == ScreenMode::Reward)
	{
		DrawRewardScreen(&memDc);
		dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
		memDc.SelectObject(previousBitmap);
		return;
	}
	if (_screenMode == ScreenMode::Result)
	{
		DrawResultScreen(&memDc);
		dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
		memDc.SelectObject(previousBitmap);
		return;
	}

	//targetball
	auto& targets = _game.GetTargets()._targetBallList;
	auto pos = targets.GetHeadPosition();
	while (pos != nullptr)
	{
		auto& _target = targets.GetNext(pos);
		_target.draw(&memDc, _options.pegColorMode);
	}

	DrawOrbTrail(&memDc);

	//player
	_game.GetPlayer().draw(
		&memDc,
		_playerSprite.GetSafeHandle() != nullptr ? &_playerSprite : nullptr);


	//enemy squad
	const auto& enemyRoster = _game.GetEnemies();
	const bool enemyGroup = enemyRoster.size() > 1;
	for (std::size_t index = 0; index < enemyRoster.size(); ++index)
	{
		const EnemyCombatant& combatant = enemyRoster[index];
		if (!combatant.IsAlive())
		{
			continue;
		}
		combatant.actor.draw(
			&memDc,
			GetEnemySprite(combatant.definition.visual),
			enemyGroup ? GameLayout::EnemyGroupSize : GameLayout::EnemySize,
			enemyGroup ? GameLayout::EnemyGroupY : GameLayout::EnemyInitialPosition.y,
			index == _game.GetActiveEnemyIndex());
		DrawEnemyHealthBar(
			&memDc,
			combatant,
			enemyGroup ? GameLayout::EnemyGroupSize : GameLayout::EnemySize,
			enemyGroup ? GameLayout::EnemyGroupY : GameLayout::EnemyInitialPosition.y,
			enemyGroup,
			index == _game.GetActiveEnemyIndex());
	}

	float orbOffsetY = 0.0f;
	float orbScale = 1.0f;
	if (_game.GetState() == GameState::Aiming && !_game.GetBall().GetClick())
	{
		orbOffsetY = std::sin(_gameplayVisualTimeSeconds * 3.2f) * 4.0f;
		orbScale += std::sin(_gameplayVisualTimeSeconds * 4.4f) * 0.055f;
	}
	_game.GetBall().draw(
		&memDc,
		_orbSprite.GetSafeHandle() != nullptr ? &_orbSprite : nullptr,
		orbOffsetY,
		orbScale);

	DrawAimPreview(&memDc);

	const int statusTextState = memDc.SaveDC();
	memDc.SetBkMode(TRANSPARENT);
	memDc.SetTextColor(RGB(238, 232, 211));
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
		Text1.Format(
			_T("%s HP %d/%d · 남은 %zu"),
			Utf8Text(_game.GetActiveEnemyDefinition().displayName).GetString(),
			static_cast<int>(std::lround(_game.GetEnemy().GetHp())),
			static_cast<int>(std::lround(_game.GetActiveEnemyDefinition().health)),
			_game.GetLivingEnemyCount());
		memDc.TextOut(
			680,
			static_cast<int>(std::lround(GameLayout::EnemyHealthTextY)),
			Text1);
		CString enemyAction = EnemyActionText(_game.GetNextEnemyAction());
		if (_game.GetEnemyShield() > 0.0f)
		{
			CString shieldText;
			shieldText.Format(
				_T(" · 방어막 %d"),
				static_cast<int>(std::lround(_game.GetEnemyShield())));
			enemyAction += shieldText;
		}
		memDc.TextOut(
			680,
			static_cast<int>(std::lround(GameLayout::EnemyHealthTextY + 20.0f)),
			enemyAction);
	}
	memDc.RestoreDC(statusTextState);

	const int textState = memDc.SaveDC();
	memDc.SetBkMode(TRANSPARENT);
	memDc.SetTextColor(RGB(238, 232, 211));
	memDc.TextOut(
		static_cast<int>(std::lround(GameLayout::StateText.x)),
		static_cast<int>(std::lround(GameLayout::StateText.y)),
		StateText(_game.GetState()));
	memDc.TextOut(
		static_cast<int>(std::lround(GameLayout::FeedbackText.x)),
		static_cast<int>(std::lround(GameLayout::FeedbackText.y)),
		FeedbackText(_game.GetFeedback(), _game.GetScore()));
	CString optionsText;
	optionsText.Format(
		_T("%s · %s (M) · %s"),
		DifficultyText(_game.GetDifficulty()).GetString(),
		_options.soundEnabled ? _T("소리") : _T("음소거"),
		_options.pegColorMode == PegColorMode::HighContrast ? _T("고대비") : _T("표준"));
	memDc.TextOut(
		static_cast<int>(std::lround(GameLayout::OptionsText.x)),
		static_cast<int>(std::lround(GameLayout::OptionsText.y)),
		optionsText);
	memDc.RestoreDC(textState);
	DrawPlayingLoadout(&memDc);

	DrawFeedbackAnimations(&memDc);

	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
	memDc.SelectObject(previousBitmap);
	// 그리기 메시지에 대해서는 CWnd::OnPaint()를 호출하지 마십시오.
}



int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	_uiBackgroundLoaded = _uiBackground.LoadBitmap(IDB_UI_ADVENTURE_FRAME) != FALSE;
	_gameplayBackground.LoadBitmap(IDB_GAMEPLAY_CAVE_V3);
	_playerSprite.LoadBitmap(IDB_PLAYER_HERO_V2);
	_enemySprite.LoadBitmap(IDB_ENEMY_CRYSTAL_TOAD_V2);
	_enemyBatSprite.LoadBitmap(IDB_ENEMY_EMBER_BAT_V1);
	_enemyShamanSprite.LoadBitmap(IDB_ENEMY_MOSS_SHAMAN_V1);
	_orbSprite.LoadBitmap(IDB_ORB_AMBER_TEAL_V2);

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
	SaveOptions();
	if (_uiBackgroundLoaded)
	{
		_uiBackground.DeleteObject();
		_uiBackgroundLoaded = false;
	}
	if (_gameplayBackground.GetSafeHandle() != nullptr)
	{
		_gameplayBackground.DeleteObject();
	}
	if (_playerSprite.GetSafeHandle() != nullptr)
	{
		_playerSprite.DeleteObject();
	}
	if (_enemySprite.GetSafeHandle() != nullptr)
	{
		_enemySprite.DeleteObject();
	}
	if (_enemyBatSprite.GetSafeHandle() != nullptr)
	{
		_enemyBatSprite.DeleteObject();
	}
	if (_enemyShamanSprite.GetSafeHandle() != nullptr)
	{
		_enemyShamanSprite.DeleteObject();
	}
	if (_orbSprite.GetSafeHandle() != nullptr)
	{
		_orbSprite.DeleteObject();
	}

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
	if (_screenMode != ScreenMode::Playing)
	{
		UpdateFeedbackAnimations(deltaSeconds);
		return;
	}

	const GameUpdateResult result = _game.Update(deltaSeconds);
	ConsumeGameEvents();
	UpdateFeedbackAnimations(deltaSeconds);
	UpdateOrbVisuals(deltaSeconds);

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
		animation.color = PegEffectColor(event.pegType, _options.pegColorMode);

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
		case GameEventType::EnemyAdvanced:
			animation.text.Format(_T("ADVANCE %d"), static_cast<int>(std::lround(event.damage)));
			animation.color = RGB(240, 180, 80);
			break;
		case GameEventType::EnemyFortified:
			animation.text.Format(_T("SHIELD +%d"), static_cast<int>(std::lround(event.damage)));
			animation.color = RGB(90, 180, 255);
			animation.lifetimeSeconds = 1.1f;
			break;
		case GameEventType::EnemyDefeated:
			animation.text.Format(_T("DEFEATED · %d LEFT"), event.affectedPegs);
			animation.color = RGB(255, 194, 62);
			animation.lifetimeSeconds = 1.25f;
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

void CChildView::UpdateOrbVisuals(float deltaSeconds)
{
	_gameplayVisualTimeSeconds += deltaSeconds;
	for (OrbTrailPoint& point : _orbTrail)
	{
		point.ageSeconds += deltaSeconds;
	}
	constexpr float TRAIL_LIFETIME_SECONDS = 0.24f;
	_orbTrail.erase(
		std::remove_if(
			_orbTrail.begin(),
			_orbTrail.end(),
			[](const OrbTrailPoint& point)
			{
				return point.ageSeconds >= TRAIL_LIFETIME_SECONDS;
			}),
		_orbTrail.end());

	if (_game.GetState() != GameState::BallInFlight)
	{
		_orbTrail.clear();
		_orbTrailSampleSeconds = 0.0f;
		return;
	}

	constexpr float TRAIL_SAMPLE_SECONDS = 0.025f;
	_orbTrailSampleSeconds += deltaSeconds;
	if (_orbTrailSampleSeconds < TRAIL_SAMPLE_SECONDS)
	{
		return;
	}
	_orbTrailSampleSeconds = 0.0f;

	const Vector2 position = _game.GetBall().GetPosition();
	if (!_orbTrail.empty() && (position - _orbTrail.back().position).Length() < 3.0f)
	{
		return;
	}
	_orbTrail.push_back({ position, 0.0f });
	constexpr std::size_t MAX_TRAIL_POINTS = 12;
	if (_orbTrail.size() > MAX_TRAIL_POINTS)
	{
		_orbTrail.erase(_orbTrail.begin());
	}
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

void CChildView::DrawOrbTrail(CDC* deviceContext)
{
	if (_orbTrail.empty())
	{
		return;
	}

	constexpr float TRAIL_LIFETIME_SECONDS = 0.24f;
	const int savedDc = deviceContext->SaveDC();
	deviceContext->SelectObject(GetStockObject(NULL_PEN));
	for (const OrbTrailPoint& point : _orbTrail)
	{
		const float strength = std::clamp(
			1.0f - point.ageSeconds / TRAIL_LIFETIME_SECONDS,
			0.0f,
			1.0f);
		const int radius = static_cast<int>(std::lround(3.0f + strength * 6.0f));
		const BYTE red = static_cast<BYTE>(std::lround(28.0f + strength * 45.0f));
		const BYTE green = static_cast<BYTE>(std::lround(80.0f + strength * 145.0f));
		const BYTE blue = static_cast<BYTE>(std::lround(105.0f + strength * 130.0f));
		CBrush trailBrush(RGB(red, green, blue));
		CBrush* previousBrush = deviceContext->SelectObject(&trailBrush);
		const int centerX = static_cast<int>(std::lround(point.position.x));
		const int centerY = static_cast<int>(std::lround(point.position.y));
		deviceContext->Ellipse(
			centerX - radius,
			centerY - radius,
			centerX + radius,
			centerY + radius);
		deviceContext->SelectObject(previousBrush);
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

	CPen pathPen(PS_SOLID, 2, RGB(190, 220, 255));
	CPen reflectedPen(PS_SOLID, 3, RGB(255, 194, 62));
	deviceContext->SelectObject(&pathPen);
	deviceContext->SelectObject(GetStockObject(NULL_BRUSH));
	deviceContext->MoveTo(
		static_cast<int>(std::lround(_game.GetBall().GetPosition().x)),
		static_cast<int>(std::lround(_game.GetBall().GetPosition().y)));
	for (std::size_t index = 0; index < preview.points.size(); ++index)
	{
		const Vector2 point = preview.points[index];
		deviceContext->LineTo(
			static_cast<int>(std::lround(point.x)),
			static_cast<int>(std::lround(point.y)));
		if (index == preview.firstPegCollisionPoint)
		{
			deviceContext->SelectObject(&reflectedPen);
		}
	}

	if (preview.PredictsPegCollision())
	{
		const Vector2 collision = preview.points[preview.firstPegCollisionPoint];
		const int collisionX = static_cast<int>(std::lround(collision.x));
		const int collisionY = static_cast<int>(std::lround(collision.y));
		deviceContext->SelectObject(&reflectedPen);
		deviceContext->SelectObject(GetStockObject(NULL_BRUSH));
		deviceContext->Ellipse(
			collisionX - 7,
			collisionY - 7,
			collisionX + 7,
			collisionY + 7);
	}

	const Vector2 arrowEnd = preview.points.back();
	const Vector2 arrowDirection =
		(arrowEnd - preview.points[preview.points.size() - 2]).Normalized();
	const Vector2 arrowNormal{ -arrowDirection.y, arrowDirection.x };
	const Vector2 arrowLeft = arrowEnd - arrowDirection * 9.0f + arrowNormal * 4.0f;
	const Vector2 arrowRight = arrowEnd - arrowDirection * 9.0f - arrowNormal * 4.0f;
	deviceContext->MoveTo(
		static_cast<int>(std::lround(arrowLeft.x)),
		static_cast<int>(std::lround(arrowLeft.y)));
	deviceContext->LineTo(
		static_cast<int>(std::lround(arrowEnd.x)),
		static_cast<int>(std::lround(arrowEnd.y)));
	deviceContext->LineTo(
		static_cast<int>(std::lround(arrowRight.x)),
		static_cast<int>(std::lround(arrowRight.y)));

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

void CChildView::DrawEnemyHealthBar(
	CDC* deviceContext,
	const EnemyCombatant& combatant,
	Vector2 drawSize,
	float drawY,
	bool enemyGroup,
	bool activeTarget)
{
	const float fraction = combatant.HealthFraction();
	const COLORREF fillColor = fraction <= 0.25f
		? UiTheme::Danger
		: (fraction <= 0.55f ? UiTheme::Orange : UiTheme::Green);
	const float barTop = enemyGroup
		? drawY + GameLayout::EnemyGroupHealthBarOffsetY
		: drawY + drawSize.y + GameLayout::EnemySoloHealthBarOffsetY;
	const int left = static_cast<int>(std::lround(
		combatant.actor.GetX() + GameLayout::EnemyHealthBarInsetX));
	const int right = static_cast<int>(std::lround(
		combatant.actor.GetX() + drawSize.x - GameLayout::EnemyHealthBarInsetX));
	const int top = static_cast<int>(std::lround(barTop));
	const int bottom = static_cast<int>(std::lround(
		barTop + GameLayout::EnemyHealthBarHeight));

	CString healthText;
	healthText.Format(
		_T("%d/%d"),
		static_cast<int>(std::lround(combatant.actor.GetHp())),
		static_cast<int>(std::lround(combatant.definition.health)));
	UiRenderer::DrawProgressBar(
		deviceContext,
		CRect(left, top, right, bottom),
		fraction,
		healthText,
		fillColor,
		activeTarget ? UiTheme::Gold : UiTheme::Border);
}

void CChildView::DrawStageSelection(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	UiRenderer::DrawText(deviceContext, CRect(180, 35, 800, 110), _T("ADVENTURE RUN"), 300, UiTheme::Gold);

	UiRenderer::DrawPanel(deviceContext, CRect(28, 150, 222, 625));
	UiRenderer::DrawText(deviceContext, CRect(42, 165, 208, 210), _T("RUN STATUS"), 155, UiTheme::Gold);
	CString runProgress;
	runProgress.Format(
		_T("스테이지 %zu / %zu\n체력 %d"),
		_run.GetCurrentStageIndex() + 1,
		_run.GetStageCount(),
		static_cast<int>(std::lround(_runPlayerHealth > 0.0f ? _runPlayerHealth : _game.GetPlayer().GetHp())));
	UiRenderer::DrawText(deviceContext, CRect(42, 215, 208, 275), runProgress, 105, UiTheme::Green, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawText(deviceContext, CRect(42, 285, 208, 315), _T("현재 오브"), 105, UiTheme::MutedText);
	UiRenderer::DrawText(
		deviceContext,
		CRect(42, 315, 208, 355),
		Utf8Text(_game.GetLoadout().GetSelectedOrb().displayName),
		145,
		UiTheme::Text);
	UiRenderer::DrawText(deviceContext, CRect(42, 365, 208, 395), _T("보유 유물"), 105, UiTheme::MutedText);
	UiRenderer::DrawText(
		deviceContext,
		CRect(45, 400, 205, 525),
		RelicSummary(_game.GetLoadout()),
		110,
		UiTheme::Green,
		DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawKeyHint(deviceContext, CRect(48, 550, 202, 600), _T("[L] 장비 관리"));

	const std::size_t visibleStageCount = (std::min)(std::size_t{ 3 }, _contentCatalog.stages.size());
	if (visibleStageCount > 0 && _selectedStageIndex >= visibleStageCount)
	{
		_selectedStageIndex = 0;
	}
	for (std::size_t index = 0; index < visibleStageCount; ++index)
	{
		const StageDefinition configured = ApplyDifficulty(_contentCatalog.stages[index], _options.difficulty);
		const int top = 150 + static_cast<int>(index) * 160;
		const CRect card(248, top, 752, top + 135);
		const bool completed = index < _run.GetClearedStageCount();
		const bool selected = index == _run.GetCurrentStageIndex()
			&& _run.GetStatus() == RunStatus::StageReady;
		const COLORREF stageColor = completed
			? UiTheme::Green
			: (configured.isBoss ? UiTheme::Orange : UiTheme::Gold);
		UiRenderer::DrawPanel(deviceContext, card, selected || completed, stageColor);
		CString title;
		title.Format(
			_T("[%zu] %s%s"),
			index + 1,
			Utf8Text(configured.displayName).GetString(),
			configured.isBoss ? _T(" · BOSS") : _T(""));
		UiRenderer::DrawText(
			deviceContext,
			CRect(card.left + 15, card.top + 10, card.right - 15, card.top + 58),
			title,
			160,
			completed ? UiTheme::Green : (selected ? UiTheme::Gold : UiTheme::MutedText));
		const std::size_t enemyCount = configured.enemies.empty()
			? std::size_t{ 1 }
			: configured.enemies.size();
		float totalEnemyHealth = configured.enemies.empty()
			? configured.rules.enemyHealth
			: 0.0f;
		for (const EnemyDefinition& enemy : configured.enemies)
		{
			totalEnemyHealth += enemy.health;
		}
		CString rules;
		rules.Format(
			_T("페그 %zu · 적 %zu · 총 HP %d · %s"),
			configured.pegLayout.pegs.size(),
			enemyCount,
			static_cast<int>(std::lround(totalEnemyHealth)),
			configured.isBoss ? _T("행동 패턴") : _T("이동 후 공격"));
		UiRenderer::DrawText(
			deviceContext,
			CRect(card.left + 15, card.top + 55, card.right - 15, card.top + 92),
			rules,
			115,
			UiTheme::MutedText);
		const StageRecord record = _records.Get(configured.id, _options.difficulty);
		CString recordText;
		const CString nodeState = completed
			? _T("CLEAR")
			: (selected ? _T("NEXT") : _T("LOCKED"));
		recordText.Format(
			_T("%s · 기록 %d · 콤보 %d · 클리어 %d"),
			nodeState.GetString(),
			record.highScore,
			record.bestCombo,
			record.clearCount);
		UiRenderer::DrawText(
			deviceContext,
			CRect(card.left + 15, card.top + 92, card.right - 15, card.bottom - 8),
			recordText,
			105,
			UiTheme::Green);
	}

	UiRenderer::DrawPanel(deviceContext, CRect(778, 150, 968, 625));
	UiRenderer::DrawText(deviceContext, CRect(792, 165, 954, 210), _T("RUN RULES"), 155, UiTheme::Gold);
	CString difficulty;
	difficulty.Format(_T("난이도\n%s"), DifficultyText(_options.difficulty).GetString());
	UiRenderer::DrawText(deviceContext, CRect(792, 235, 954, 310), difficulty, 125, UiTheme::Text, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	CString sound;
	sound.Format(_T("사운드 %s"), _options.soundEnabled ? _T("켜짐") : _T("꺼짐"));
	UiRenderer::DrawText(deviceContext, CRect(792, 330, 954, 370), sound, 115, UiTheme::MutedText);
	UiRenderer::DrawText(deviceContext, CRect(792, 380, 954, 445), PegColorModeText(_options.pegColorMode), 110, UiTheme::MutedText, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawKeyHint(deviceContext, CRect(798, 550, 948, 600), _T("[O] 옵션"));

	UiRenderer::DrawKeyHint(deviceContext, CRect(300, 640, 680, 690), _T("현재 노드 시작 · ENTER"));
	if (!_contentCatalog.UsedExternalContent())
	{
		UiRenderer::DrawText(deviceContext, CRect(220, 610, 760, 638), _T("외부 콘텐츠 오류 · 검증된 내장 카탈로그 사용 중"), 95, UiTheme::Orange);
	}
}

void CChildView::DrawRewardScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	UiRenderer::DrawText(deviceContext, CRect(180, 35, 800, 110), _T("STAGE REWARD"), 300, UiTheme::Gold);
	UiRenderer::DrawText(deviceContext, CRect(160, 125, 820, 175), _T("보상 하나를 선택하면 다음 스테이지가 열립니다"), 125, UiTheme::MutedText);

	const auto& rewards = _run.GetRewardChoices();
	for (std::size_t index = 0; index < rewards.size(); ++index)
	{
		const RunReward& reward = rewards[index];
		const int left = 75 + static_cast<int>(index) * 305;
		const CRect card(left, 245, left + 270, 485);
		const COLORREF color = reward.kind == RunRewardKind::Orb
			? UiTheme::Blue
			: (reward.kind == RunRewardKind::Relic ? UiTheme::Gold : UiTheme::Green);
		UiRenderer::DrawPanel(deviceContext, card, true, color);
		CString category;
		switch (reward.kind)
		{
		case RunRewardKind::Orb: category = _T("ORB"); break;
		case RunRewardKind::Relic: category = _T("RELIC"); break;
		case RunRewardKind::Heal: category = _T("RECOVER"); break;
		}
		CString title;
		title.Format(_T("[%zu] %s"), index + 1, category.GetString());
		UiRenderer::DrawText(deviceContext, CRect(left + 15, 265, left + 255, 320), title, 160, color);
		UiRenderer::DrawText(deviceContext, CRect(left + 15, 335, left + 255, 420), Utf8Text(reward.displayName), 145, UiTheme::Text, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	}
	UiRenderer::DrawText(deviceContext, CRect(150, 535, 830, 585), _runNotice, 110, UiTheme::Orange);
	UiRenderer::DrawKeyHint(deviceContext, CRect(260, 640, 720, 690), _T("보상 카드 클릭 · 1/2/3 선택"));
}

void CChildView::DrawLoadoutScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	UiRenderer::DrawText(deviceContext, CRect(160, 35, 820, 110), _T("ORB & RELIC LOADOUT"), 285, UiTheme::Gold);
	UiRenderer::DrawText(deviceContext, CRect(80, 125, 900, 165), _T("보유 오브 목록 · 선택한 오브는 다음 전투의 첫 순서로 우선됩니다"), 115, UiTheme::MutedText);

	const auto& orbs = GetOrbDefinitions();
	const auto& ownedOrbs = _game.GetLoadout().GetOwnedOrbs();
	for (std::size_t index = 0; index < orbs.size(); ++index)
	{
		const OrbDefinition& orb = orbs[index];
		const int left = 75 + static_cast<int>(index) * 305;
		const CRect card(left, 185, left + 270, 335);
		const bool selected = _game.GetLoadout().GetSelectedOrbId() == orb.id;
		const std::size_t ownedCount = static_cast<std::size_t>(std::count_if(
			ownedOrbs.begin(),
			ownedOrbs.end(),
			[&orb](const std::string& id) { return id == orb.id; }));
		UiRenderer::DrawPanel(deviceContext, card, selected, UiTheme::Gold);
		DrawOrbIcon(deviceContext, CRect(left + 20, 204, left + 52, 236), orb.id);
		CString title;
		title.Format(_T("[%zu] %s · x%zu"), index + 1, Utf8Text(orb.displayName).GetString(), ownedCount);
		UiRenderer::DrawText(deviceContext, CRect(left + 58, 200, left + 260, 250), title, 125, selected ? UiTheme::Gold : UiTheme::Text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		CString stats;
		stats.Format(_T("피해 x%.2f\n점수 x%.2f"), orb.pegDamageMultiplier, orb.scoreMultiplier);
		UiRenderer::DrawText(deviceContext, CRect(left + 15, 255, left + 255, 320), stats, 110, UiTheme::MutedText, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	}

	const auto& relics = GetRelicDefinitions();
	for (std::size_t index = 0; index < relics.size(); ++index)
	{
		const RelicDefinition& relic = relics[index];
		const int left = 75 + static_cast<int>(index) * 305;
		const CRect card(left, 390, left + 270, 555);
		const std::size_t stacks = _game.GetLoadout().GetRelicStackCount(relic.id);
		const bool atLimit = stacks >= relic.maxStacks;
		UiRenderer::DrawPanel(deviceContext, card, atLimit, UiTheme::Green);
		CString title;
		title.Format(_T("[%zu] %s"), index + 4, Utf8Text(relic.displayName).GetString());
		UiRenderer::DrawText(deviceContext, CRect(left + 10, 405, left + 260, 450), title, 140, atLimit ? UiTheme::Green : UiTheme::Text);
		CString stats;
		stats.Format(
			_T("보유 %zu / %zu\n피해 x%.2f · 점수 x%.2f\n받는 피해 x%.2f"),
			stacks,
			relic.maxStacks,
			relic.pegDamageMultiplier,
			relic.scoreMultiplier,
			relic.incomingDamageMultiplier);
		UiRenderer::DrawText(deviceContext, CRect(left + 12, 455, left + 258, 540), stats, 95, UiTheme::MutedText, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	}

	const ProgressionModifiers modifiers = _game.GetProgressionModifiers();
	CString total;
	total.Format(
		_T("최종 배율 · 페그 피해 x%.2f · 점수 x%.2f · 받는 피해 x%.2f"),
		modifiers.pegDamageMultiplier,
		modifiers.scoreMultiplier,
		modifiers.incomingDamageMultiplier);
	UiRenderer::DrawText(deviceContext, CRect(140, 565, 840, 605), total, 115, UiTheme::Green);
	UiRenderer::DrawText(deviceContext, CRect(140, 605, 840, 635), _loadoutNotice, 100, UiTheme::Orange);
	UiRenderer::DrawKeyHint(deviceContext, CRect(75, 640, 300, 690), _T("초기화 · X"));
	UiRenderer::DrawText(deviceContext, CRect(305, 642, 675, 688), _T("카드 클릭 또는 1-6 키"), 100, UiTheme::MutedText);
	UiRenderer::DrawKeyHint(deviceContext, CRect(680, 640, 905, 690), _T("돌아가기 · ESC"));
}

void CChildView::DrawOptions(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	UiRenderer::DrawPanel(deviceContext, CRect(235, 125, 745, 670));
	UiRenderer::DrawText(deviceContext, CRect(300, 55, 680, 120), _T("OPTIONS"), 300, UiTheme::Gold);
	CString difficulty;
	difficulty.Format(_T("[D] 난이도    %s"), DifficultyText(_options.difficulty).GetString());
	UiRenderer::DrawPanel(deviceContext, CRect(285, 205, 695, 285));
	UiRenderer::DrawText(deviceContext, CRect(300, 215, 680, 275), difficulty, 165);
	CString sound;
	sound.Format(_T("[M] 사운드    %s"), _options.soundEnabled ? _T("켜짐") : _T("꺼짐"));
	UiRenderer::DrawPanel(deviceContext, CRect(285, 315, 695, 395));
	UiRenderer::DrawText(deviceContext, CRect(300, 325, 680, 385), sound, 165);
	CString colorMode;
	colorMode.Format(_T("[C] 페그 구분    %s"), PegColorModeText(_options.pegColorMode).GetString());
	UiRenderer::DrawPanel(deviceContext, CRect(285, 425, 695, 505));
	UiRenderer::DrawText(deviceContext, CRect(300, 435, 680, 495), colorMode, 155);
	UiRenderer::DrawText(deviceContext, CRect(275, 530, 705, 565), _T("쉬움: 적 체력·공격 75~80% · 행동 지연"), 105, UiTheme::MutedText);
	UiRenderer::DrawText(deviceContext, CRect(275, 565, 705, 600), _T("어려움: 적 체력 150% · 공격 125%"), 105, UiTheme::MutedText);
	UiRenderer::DrawText(deviceContext, CRect(275, 610, 705, 645), _settingsSaveFailed ? _T("설정 저장 실패 · 현재 실행은 계속됩니다") : _T("변경 내용 자동 저장"), 105, _settingsSaveFailed ? UiTheme::Danger : UiTheme::Green);
	UiRenderer::DrawKeyHint(deviceContext, CRect(300, 640, 680, 690), _T("[B] 또는 ESC로 돌아가기"));
}

void CChildView::DrawResultScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	const bool victory = _resultSummary.has_value()
		&& _resultSummary->result == GameUpdateResult::Victory;
	UiRenderer::DrawPanel(deviceContext, CRect(250, 135, 730, 650), true, victory ? UiTheme::Green : UiTheme::Danger);
	UiRenderer::DrawText(deviceContext, CRect(250, 55, 730, 125), victory ? _T("RUN COMPLETE") : _T("RUN FAILED"), 300, victory ? UiTheme::Green : UiTheme::Danger);
	if (_resultSummary.has_value())
	{
		UiRenderer::DrawText(deviceContext, CRect(280, 175, 700, 225), Utf8Text(_resultSummary->stageName), 165, UiTheme::Gold);
		CString scoreText;
		scoreText.Format(_T("SCORE  %d"), _resultSummary->totalScore);
		UiRenderer::DrawText(deviceContext, CRect(280, 275, 700, 325), scoreText, 180);
		CString comboText;
		comboText.Format(_T("BEST COMBO  %d"), _resultSummary->bestCombo);
		UiRenderer::DrawText(deviceContext, CRect(280, 350, 700, 400), comboText, 160);
		CString turnText;
		turnText.Format(_T("TURNS  %d"), _resultSummary->turns);
		UiRenderer::DrawText(deviceContext, CRect(280, 420, 700, 470), turnText, 150);
		const StageRecord record = _records.Get(_resultSummary->stageId, _options.difficulty);
		CString recordText;
		recordText.Format(
			_T("RECORD %d · COMBO %d · CLEARS %d"),
			record.highScore,
			record.bestCombo,
			record.clearCount);
		UiRenderer::DrawText(deviceContext, CRect(280, 500, 700, 545), recordText, 115, UiTheme::Green);
	}
	if (_recordSaveFailed)
	{
		UiRenderer::DrawText(deviceContext, CRect(275, 565, 705, 610), _T("기록 저장 실패 · 현재 실행에서만 유지"), 105, UiTheme::Danger);
	}
	UiRenderer::DrawKeyHint(deviceContext, CRect(260, 640, 480, 690), _T("다시 도전 · R"));
	UiRenderer::DrawKeyHint(deviceContext, CRect(500, 640, 720, 690), _T("새 런 · S"));
}

void CChildView::DrawMenuBackdrop(CDC* deviceContext)
{
	CRect bounds;
	GetClientRect(&bounds);
	UiRenderer::DrawBackdrop(
		deviceContext,
		_uiBackgroundLoaded ? &_uiBackground : nullptr,
		bounds);
}

CBitmap* CChildView::GetEnemySprite(EnemyVisualKind visual) noexcept
{
	switch (visual)
	{
	case EnemyVisualKind::CrystalToad:
		return _enemySprite.GetSafeHandle() != nullptr ? &_enemySprite : nullptr;
	case EnemyVisualKind::EmberBat:
		return _enemyBatSprite.GetSafeHandle() != nullptr ? &_enemyBatSprite : nullptr;
	case EnemyVisualKind::MossShaman:
		return _enemyShamanSprite.GetSafeHandle() != nullptr ? &_enemyShamanSprite : nullptr;
	}
	return nullptr;
}

void CChildView::DrawPlayingLoadout(CDC* deviceContext)
{
	const PlayerLoadout& loadout = _game.GetLoadout();
	UiRenderer::DrawPanel(deviceContext, CRect(245, 128, 735, 188));
	DrawOrbIcon(deviceContext, CRect(262, 137, 290, 165), loadout.GetSelectedOrbId());
	DrawOrbIcon(deviceContext, CRect(487, 137, 515, 165), loadout.GetNextOrbId());
	CString currentText;
	currentText.Format(_T("현재  %s"), Utf8Text(loadout.GetSelectedOrb().displayName).GetString());
	UiRenderer::DrawText(deviceContext, CRect(298, 132, 478, 165), currentText, 90, UiTheme::Gold, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	CString nextText;
	nextText.Format(_T("다음  %s"), Utf8Text(loadout.GetNextOrb().displayName).GetString());
	UiRenderer::DrawText(deviceContext, CRect(523, 132, 720, 165), nextText, 90, UiTheme::Blue, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	CString pileText;
	pileText.Format(
		_T("덱 %zu · 버림 %zu · 리필 %zu · %s"),
		loadout.GetDrawPileCount(),
		loadout.GetDiscardPileCount(),
		loadout.GetReloadCount(),
		RelicSummary(loadout).GetString());
	UiRenderer::DrawText(deviceContext, CRect(260, 162, 720, 185), pileText, 75, UiTheme::MutedText);
}

bool CChildView::StartStage(std::string_view stageId)
{
	const StageDefinition* stage = FindContentStage(_contentCatalog.stages, stageId);
	if (stage == nullptr || !_game.LoadStage(*stage, _runDifficulty))
	{
		return false;
	}

	_feedbackAnimations.clear();
	_orbTrail.clear();
	_orbTrailSampleSeconds = 0.0f;
	_gameplayVisualTimeSeconds = 0.0f;
	_resultSummary.reset();
	if (_run.GetClearedStageCount() > 0 && _runPlayerHealth > 0.0f)
	{
		_game.GetPlayer().SetHp((std::min)(_runPlayerHealth, _game.GetStage().rules.playerHealth));
	}
	_runPlayerHealth = _game.GetPlayer().GetHp();
	_screenMode = ScreenMode::Playing;
	SetFocus();
	return true;
}

bool CChildView::StartSelectedStage()
{
	const std::size_t stageCount = (std::min)(std::size_t{ 3 }, _contentCatalog.stages.size());
	return stageCount > 0
		&& _run.GetStatus() == RunStatus::StageReady
		&& _selectedStageIndex == _run.GetCurrentStageIndex()
		&& _selectedStageIndex < stageCount
		&& StartStage(_contentCatalog.stages[_selectedStageIndex].id);
}

void CChildView::BeginNewRun()
{
	std::vector<std::string> stageIds;
	const std::size_t stageCount = (std::min)(std::size_t{ 3 }, _contentCatalog.stages.size());
	stageIds.reserve(stageCount);
	for (std::size_t index = 0; index < stageCount; ++index)
	{
		stageIds.push_back(_contentCatalog.stages[index].id);
	}

	_game.ResetProgression();
	_run.Start(std::move(stageIds));
	_runDifficulty = _options.difficulty;
	_runPlayerHealth = 0.0f;
	_selectedStageIndex = 0;
	_runNotice.Empty();
	_resultSummary.reset();
	_feedbackAnimations.clear();
	_orbTrail.clear();
	_screenMode = ScreenMode::StageSelection;
}

bool CChildView::SelectRunReward(std::size_t index)
{
	const std::optional<RunReward> selected = _run.SelectReward(index);
	if (!selected.has_value())
	{
		return false;
	}

	switch (selected->kind)
	{
	case RunRewardKind::Orb:
		_game.AddOrb(selected->id);
		break;
	case RunRewardKind::Relic:
		_game.AcquireRelic(selected->id);
		break;
	case RunRewardKind::Heal:
		_runPlayerHealth = (std::min)(
			_runPlayerHealth + selected->magnitude,
			_game.GetStage().rules.playerHealth);
		break;
	}

	_runNotice.Format(_T("%s 선택 완료"), Utf8Text(selected->displayName).GetString());
	_selectedStageIndex = _run.GetCurrentStageIndex();
	_screenMode = ScreenMode::StageSelection;
	return true;
}

void CChildView::SaveOptions()
{
	_settingsSaveFailed = !_settingsStore.Save(_options);
}

void CChildView::RecordResult(bool cleared)
{
	if (!_resultSummary.has_value())
	{
		return;
	}

	if (_records.ApplyResult(
		_resultSummary->stageId,
		_options.difficulty,
		_resultSummary->totalScore,
		_resultSummary->bestCombo,
		cleared))
	{
		_recordSaveFailed = !_recordStore.Save(_records);
	}
}

void CChildView::PlayEventSound(GameEventType eventType, PegType pegType)
{
	if (!_options.soundEnabled)
	{
		return;
	}

	UINT sound = MB_OK;
	if (eventType == GameEventType::PlayerDamaged || eventType == GameEventType::Defeat)
	{
		sound = MB_ICONHAND;
	}
	else if (eventType == GameEventType::Victory
		|| eventType == GameEventType::EnemyDefeated
		|| pegType == PegType::Refresh)
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
	if (_screenMode != ScreenMode::Playing)
	{
		if (HandleMenuClick(point))
		{
			SetFocus();
			Invalidate(FALSE);
		}
		CWnd::OnLButtonDown(nFlags, point);
		return;
	}

	if (_game.BeginAim({ static_cast<float>(point.x), static_cast<float>(point.y) }))
	{
		SetFocus();
		SetCapture();
	}
	CWnd::OnLButtonDown(nFlags, point);
}

bool CChildView::HandleMenuClick(CPoint point)
{
	UiScreenKind screen = UiScreenKind::StageSelection;
	switch (_screenMode)
	{
	case ScreenMode::StageSelection: screen = UiScreenKind::StageSelection; break;
	case ScreenMode::Loadout: screen = UiScreenKind::Loadout; break;
	case ScreenMode::Options: screen = UiScreenKind::Options; break;
	case ScreenMode::Reward: screen = UiScreenKind::Reward; break;
	case ScreenMode::Result: screen = UiScreenKind::Result; break;
	case ScreenMode::Playing: return false;
	}

	const std::size_t stageCount = (std::min)(std::size_t{ 3 }, _contentCatalog.stages.size());
	const UiAction action = ResolveUiClick(
		screen,
		{ static_cast<float>(point.x), static_cast<float>(point.y) },
		stageCount);
	if (!action.IsHandled())
	{
		return false;
	}

	ExecuteUiAction(action);
	return true;
}

void CChildView::ExecuteUiAction(const UiAction& action)
{
	switch (action.command)
	{
	case UiCommand::SelectStage:
		if (action.index == _run.GetCurrentStageIndex()
			&& _run.GetStatus() == RunStatus::StageReady)
		{
			_selectedStageIndex = action.index;
		}
		break;
	case UiCommand::StartSelectedStage:
		StartSelectedStage();
		break;
	case UiCommand::OpenLoadout:
		_loadoutNotice.Empty();
		_screenMode = ScreenMode::Loadout;
		break;
	case UiCommand::OpenOptions:
		_screenMode = ScreenMode::Options;
		break;
	case UiCommand::SelectOrb:
	{
		const auto& orbs = GetOrbDefinitions();
		if (action.index < orbs.size() && _game.SelectOrb(orbs[action.index].id))
		{
			_loadoutNotice = _T("선택 오브를 변경했습니다");
		}
		break;
	}
	case UiCommand::AcquireRelic:
	{
		const auto& relics = GetRelicDefinitions();
		if (action.index < relics.size())
		{
			_loadoutNotice = _game.AcquireRelic(relics[action.index].id)
				? _T("유물을 획득했습니다")
				: _T("이미 획득 한도에 도달했습니다");
		}
		break;
	}
	case UiCommand::ResetProgression:
		_game.ResetProgression();
		_loadoutNotice = _T("오브와 유물을 기본 상태로 초기화했습니다");
		break;
	case UiCommand::BackToStageSelection:
		if (_screenMode == ScreenMode::Result)
		{
			BeginNewRun();
			break;
		}
		_screenMode = ScreenMode::StageSelection;
		break;
	case UiCommand::ToggleDifficulty:
		_options.CycleDifficulty();
		SaveOptions();
		break;
	case UiCommand::ToggleSound:
		_options.ToggleSound();
		SaveOptions();
		break;
	case UiCommand::TogglePegColorMode:
		_options.TogglePegColorMode();
		SaveOptions();
		break;
	case UiCommand::SelectReward:
		SelectRunReward(action.index);
		break;
	case UiCommand::RetryStage:
		_run.RetryCurrentStage();
		restart();
		break;
	case UiCommand::None:
		break;
	}
}


void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (_screenMode != ScreenMode::Playing)
	{
		CWnd::OnLButtonUp(nFlags, point);
		return;
	}

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
	if (_screenMode != ScreenMode::Playing)
	{
		CWnd::OnMouseMove(nFlags, point);
		return;
	}

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
	if (_screenMode == ScreenMode::StageSelection)
	{
		const std::size_t stageCount = (std::min)(std::size_t{ 3 }, _contentCatalog.stages.size());
		if (nChar == VK_RETURN)
		{
			StartSelectedStage();
		}
		else if (nChar >= '1' && nChar <= '3')
		{
			const std::size_t requested = static_cast<std::size_t>(nChar - '1');
			if (requested < stageCount && requested == _run.GetCurrentStageIndex())
			{
				_selectedStageIndex = requested;
				StartSelectedStage();
			}
		}
		else if (nChar == 'O')
		{
			_screenMode = ScreenMode::Options;
		}
		else if (nChar == 'L')
		{
			_loadoutNotice.Empty();
			_screenMode = ScreenMode::Loadout;
		}
		Invalidate();
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	if (_screenMode == ScreenMode::Loadout)
	{
		if (nChar >= '1' && nChar <= '3')
		{
			const std::size_t index = static_cast<std::size_t>(nChar - '1');
			const auto& orbs = GetOrbDefinitions();
			if (index < orbs.size() && _game.SelectOrb(orbs[index].id))
			{
				_loadoutNotice = _T("선택 오브를 변경했습니다");
			}
		}
		else if (nChar >= '4' && nChar <= '6')
		{
			const std::size_t index = static_cast<std::size_t>(nChar - '4');
			const auto& relics = GetRelicDefinitions();
			if (index < relics.size())
			{
				_loadoutNotice = _game.AcquireRelic(relics[index].id)
					? _T("유물을 획득했습니다")
					: _T("이미 획득 한도에 도달했습니다");
			}
		}
		else if (nChar == 'X')
		{
			_game.ResetProgression();
			_loadoutNotice = _T("오브와 유물을 기본 상태로 초기화했습니다");
		}
		else if (nChar == 'B' || nChar == VK_ESCAPE)
		{
			_screenMode = ScreenMode::StageSelection;
		}
		Invalidate();
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	if (_screenMode == ScreenMode::Options)
	{
		if (nChar == 'D')
		{
			_options.CycleDifficulty();
			SaveOptions();
		}
		else if (nChar == 'M')
		{
			_options.ToggleSound();
			SaveOptions();
		}
		else if (nChar == 'C')
		{
			_options.TogglePegColorMode();
			SaveOptions();
		}
		else if (nChar == 'B' || nChar == VK_ESCAPE)
		{
			_screenMode = ScreenMode::StageSelection;
		}
		Invalidate();
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	if (_screenMode == ScreenMode::Reward)
	{
		if (nChar >= '1' && nChar <= '3')
		{
			SelectRunReward(static_cast<std::size_t>(nChar - '1'));
		}
		Invalidate();
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	if (_screenMode == ScreenMode::Result)
	{
		if (nChar == 'R')
		{
			_run.RetryCurrentStage();
			restart();
		}
		else if (nChar == 'S')
		{
			BeginNewRun();
		}
		Invalidate();
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

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
		_options.ToggleSound();
		SaveOptions();
		Invalidate();
	}
	if (nChar == 'S' && _game.GetState() == GameState::Aiming)
	{
		ReleaseMouseInput(true);
		_game.ResetGame();
		_feedbackAnimations.clear();
		_resultSummary.reset();
		_screenMode = ScreenMode::StageSelection;
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
	BeginNewRun();
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
