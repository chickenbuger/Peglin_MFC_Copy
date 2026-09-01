
// ChildView.cpp: CChildView 클래스의 구현
//

#include "pch.h"
#include "framework.h"
#include "FinalProject_Peglin.h"
#include "ChildView.h"
#include "GameLayout.h"
#include "RewardPresentation.h"
#include <Xinput.h>
#include <algorithm>
#include <cmath>
#include <utility>

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "Xinput9_1_0.lib")

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
			text = _T("다음: 1칸 전진");
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

	CString AttackStyleText(const OrbDefinition& orb)
	{
		CString text;
		text.Format(
			_T("%s · %s"),
			orb.attackDelivery == AttackDelivery::Melee ? _T("근접") : _T("원거리"),
			orb.attackTarget == AttackTarget::All ? _T("광역") : _T("단일"));
		return text;
	}

	bool Contains(const CRect& bounds, Vector2 point) noexcept
	{
		return point.x >= static_cast<float>(bounds.left)
			&& point.y >= static_cast<float>(bounds.top)
			&& point.x < static_cast<float>(bounds.right)
			&& point.y < static_cast<float>(bounds.bottom);
	}

	CombatLogTone CombatTone(GameEventType eventType) noexcept
	{
		switch (eventType)
		{
		case GameEventType::RefreshTriggered:
		case GameEventType::RefreshGuaranteed:
		case GameEventType::RefreshRelocated:
		case GameEventType::EnemyDefeated:
		case GameEventType::Victory:
			return CombatLogTone::Positive;
		case GameEventType::BombTriggered:
		case GameEventType::PlayerAttack:
		case GameEventType::TurnResolved:
		case GameEventType::EnemyAdvanced:
		case GameEventType::EnemyFortified:
			return CombatLogTone::Warning;
		case GameEventType::PlayerDamaged:
		case GameEventType::Defeat:
			return CombatLogTone::Danger;
		case GameEventType::PegHit:
			return CombatLogTone::Neutral;
		}
		return CombatLogTone::Neutral;
	}

	bool IsCombatLogEvent(GameEventType eventType) noexcept
	{
		return eventType != GameEventType::PegHit;
	}

	COLORREF CombatToneColor(CombatLogTone tone) noexcept
	{
		switch (tone)
		{
		case CombatLogTone::Positive: return UiTheme::Green;
		case CombatLogTone::Warning: return UiTheme::Gold;
		case CombatLogTone::Danger: return UiTheme::Danger;
		case CombatLogTone::Neutral: return UiTheme::Text;
		}
		return UiTheme::Text;
	}

	void DrawMenuTitle(
		CDC* deviceContext,
		const CString& title,
		COLORREF accent = UiTheme::Gold)
	{
		const CRect banner(330, 18, 670, 76);
		UiRenderer::DrawPanel(deviceContext, banner, true, accent);
		UiRenderer::DrawText(deviceContext, banner, title, 225, accent);
	}

	void DrawOptionTile(
		CDC* deviceContext,
		const CRect& bounds,
		const CString& shortcut,
		const CString& label,
		const CString& value,
		COLORREF accent)
	{
		UiRenderer::DrawPanel(deviceContext, bounds, false, accent);
		UiRenderer::DrawText(
			deviceContext,
			CRect(bounds.left + 12, bounds.top + 10, bounds.right - 12, bounds.top + 38),
			shortcut + _T("  ") + label,
			95,
			UiTheme::MutedText);
		UiRenderer::DrawText(
			deviceContext,
			CRect(bounds.left + 12, bounds.top + 38, bounds.right - 12, bounds.bottom - 8),
			value,
			165,
			accent);
	}

	void DrawFallbackOrbIcon(CDC* deviceContext, const CRect& bounds, std::string_view orbId)
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

	void DrawFallbackRelicIcon(CDC* deviceContext, const CRect& bounds)
	{
		const int savedDc = deviceContext->SaveDC();
		CPen borderPen(PS_SOLID, 2, UiTheme::Gold);
		CBrush relicBrush(RGB(72, 118, 92));
		deviceContext->SelectObject(&borderPen);
		deviceContext->SelectObject(&relicBrush);
		POINT diamond[] = {
			{ bounds.CenterPoint().x, bounds.top },
			{ bounds.right, bounds.CenterPoint().y },
			{ bounds.CenterPoint().x, bounds.bottom },
			{ bounds.left, bounds.CenterPoint().y }
		};
		deviceContext->Polygon(diamond, 4);
		deviceContext->RestoreDC(savedDc);
	}

	void DrawHealIcon(CDC* deviceContext, const CRect& bounds)
	{
		const int savedDc = deviceContext->SaveDC();
		CBrush heartBrush(UiTheme::Green);
		deviceContext->SelectObject(&heartBrush);
		deviceContext->SelectStockObject(NULL_PEN);
		deviceContext->Ellipse(bounds);
		const int thickness = (std::max)(4, bounds.Width() / 7);
		CRect horizontal(bounds.left + bounds.Width() / 5, bounds.CenterPoint().y - thickness / 2,
			bounds.right - bounds.Width() / 5, bounds.CenterPoint().y + thickness / 2);
		CRect vertical(bounds.CenterPoint().x - thickness / 2, bounds.top + bounds.Height() / 5,
			bounds.CenterPoint().x + thickness / 2, bounds.bottom - bounds.Height() / 5);
		deviceContext->FillSolidRect(horizontal, RGB(238, 246, 224));
		deviceContext->FillSolidRect(vertical, RGB(238, 246, 224));
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

	std::filesystem::path GetDefaultGameplayCatalogPath()
	{
		std::vector<wchar_t> modulePath(1024, L'\0');
		const DWORD length = GetModuleFileNameW(
			nullptr,
			modulePath.data(),
			static_cast<DWORD>(modulePath.size()));
		if (length == 0 || length >= modulePath.size())
		{
			return std::filesystem::path(L"content") / L"gameplay.v1.ini";
		}
		return std::filesystem::path(modulePath.data()).parent_path()
			/ L"content" / L"gameplay.v1.ini";
	}

	std::filesystem::path GetDefaultLocalizationPath(UiLanguage language)
	{
		const std::filesystem::path fileName = language == UiLanguage::English
			? L"strings.en-US.v1.ini"
			: L"strings.ko-KR.v1.ini";
		std::vector<wchar_t> modulePath(1024, L'\0');
		const DWORD length = GetModuleFileNameW(
			nullptr,
			modulePath.data(),
			static_cast<DWORD>(modulePath.size()));
		if (length == 0 || length >= modulePath.size())
		{
			return std::filesystem::path(L"content") / fileName;
		}
		return std::filesystem::path(modulePath.data()).parent_path()
			/ L"content" / fileName;
	}
}

// CChildView

CChildView::CChildView()
	: _settingsStore(GetDefaultGameSettingsPath()),
	_recordStore(GetDefaultGameRecordPath())
{
	_demoRequested = HasDemoCommandLineFlag(::GetCommandLineW());
	const SettingsLoadResult settings = _settingsStore.LoadWithRecovery();
	_options = settings.options;
	if (settings.state == SettingsLoadState::Migrated)
	{
		_settingsSaveFailed = !_settingsStore.Save(_options);
	}
	else if (settings.state == SettingsLoadState::Recovered)
	{
		_optionsNotice = _T("손상된 설정을 마지막 정상 백업에서 복구했습니다");
	}
	const RecordLoadResult records = _recordStore.LoadWithRecovery();
	_records = records.records;
	if (records.state == RecordLoadState::Migrated)
	{
		_recordSaveFailed = !_recordStore.Save(_records);
	}
	else if (records.state == RecordLoadState::Recovered)
	{
		_optionsNotice = _T("손상된 전투 기록을 마지막 정상 백업에서 복구했습니다");
	}
	ReloadLocalization();
	_audioCatalog = LoadAudioCatalog(GetDefaultAudioCatalogPath());
	if (_audioCatalog.IsUsable())
	{
		_audioPlayer.SetCatalog(_audioCatalog.catalog);
	}
	ApplyAudioOptions();
	_contentCatalog = LoadContentCatalog(GetDefaultContentCatalogPath());
	_gameplayCatalog = LoadGameplayCatalog(
		GetDefaultGameplayCatalogPath(),
		_contentCatalog.stages);
	if (!ActivateGameplayCatalog(_gameplayCatalog, _contentCatalog.stages))
	{
		ResetProgressionCatalog();
	}
	BeginNewRun();
}

CChildView::~CChildView()
{
	ResetProgressionCatalog();
}

void CChildView::ReloadLocalization()
{
	_localization = LoadLocalizationCatalog(
		GetDefaultLocalizationPath(_options.language),
		_options.language);
}

CString CChildView::Text(std::string_view key) const
{
	return Utf8Text(_localization.catalog.Get(key));
}

CString CChildView::DifficultyTextForUi(GameDifficulty difficulty) const
{
	switch (difficulty)
	{
	case GameDifficulty::Easy: return Text("difficulty.easy");
	case GameDifficulty::Normal: return Text("difficulty.normal");
	case GameDifficulty::Hard: return Text("difficulty.hard");
	}

	return Text("difficulty.normal");
}

CString CChildView::PegColorModeTextForUi(PegColorMode colorMode) const
{
	return Text(colorMode == PegColorMode::HighContrast
		? "peg_color.high_contrast"
		: "peg_color.standard");
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
	ON_COMMAND(ID_GAMEPLAY_INFO, &CChildView::OnToggleGameplayInfo)
	ON_UPDATE_COMMAND_UI(ID_GAMEPLAY_INFO, &CChildView::OnUpdateGameplayInfo)
END_MESSAGE_MAP()



// CChildView 메시지 처리기

void CChildView::gameclear()
{
	_terminalTransition.Reset();
	_acquiredReward.reset();
	_rewardAcquisitionSeconds = 0.0f;
	_runNotice.Empty();
	_resultSummary = _game.GetResultSummary();
	RecordResult(true);
	_runPlayerHealth = _game.GetPlayer().GetHp();
	_run.CompleteCurrentStage();
	SetScreenMode(_run.GetStatus() == RunStatus::RewardSelection
		? ScreenMode::Reward
		: ScreenMode::Result);
	ReleaseMouseInput(true);
	_feedbackAnimations.clear();
	_attackAnimations.clear();
	_orbTrail.clear();
}

void CChildView::gameover()
{
	_terminalTransition.Reset();
	_acquiredReward.reset();
	_rewardAcquisitionSeconds = 0.0f;
	_resultSummary = _game.GetResultSummary();
	RecordResult(false);
	_run.MarkDefeated();
	SetScreenMode(ScreenMode::Result);
	ReleaseMouseInput(true);
	_feedbackAnimations.clear();
	_attackAnimations.clear();
	_orbTrail.clear();
}

void CChildView::restart()
{
	_terminalTransition.Reset();
	_acquiredReward.reset();
	_rewardAcquisitionSeconds = 0.0f;
	_game.ResetGame();
	_feedbackAnimations.clear();
	_attackAnimations.clear();
	_orbTrail.clear();
	_orbTrailSampleSeconds = 0.0f;
	_gameplayVisualTimeSeconds = 0.0f;
	_resultSummary.reset();
	SetScreenMode(ScreenMode::Playing);
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
	CRect rect;
	GetClientRect(&rect);
	const UiViewport viewport = CreateUiViewport(rect.Width(), rect.Height());
	if (!viewport.IsValid())
	{
		return;
	}
	const CRect logicalBounds(
		0,
		0,
		static_cast<int>(std::lround(viewport.logicalWidth)),
		static_cast<int>(std::lround(viewport.logicalHeight)));

	CDC memDc;
	memDc.CreateCompatibleDC(&dc);
	CBitmap  bitmap;
	bitmap.CreateCompatibleBitmap(&dc, rect.right, rect.bottom);
	CBitmap* previousBitmap = memDc.SelectObject(&bitmap);
	memDc.FillSolidRect(rect, RGB(4, 6, 10));
	const int logicalDrawingState = memDc.SaveDC();
	memDc.SetMapMode(MM_ANISOTROPIC);
	memDc.SetWindowExt(logicalBounds.Width(), logicalBounds.Height());
	memDc.SetViewportExt(viewport.pixelWidth, viewport.pixelHeight);
	memDc.SetViewportOrg(viewport.offsetX, viewport.offsetY);
	
	_background.draw(
		&memDc,
		_gameplayBackground.GetSafeHandle() != nullptr ? &_gameplayBackground : nullptr);
	auto PresentFrame = [&]()
	{
		DrawGamepadFocus(&memDc);
		DrawUiAnimations(&memDc, logicalBounds);
		DrawDemoBadge(&memDc);
		memDc.RestoreDC(logicalDrawingState);
		dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDc, 0, 0, SRCCOPY);
		memDc.SelectObject(previousBitmap);
	};

	if (_screenMode == ScreenMode::StageSelection)
	{
		DrawStageSelection(&memDc);
		PresentFrame();
		return;
	}
	if (_screenMode == ScreenMode::Loadout)
	{
		DrawLoadoutScreen(&memDc);
		PresentFrame();
		return;
	}
	if (_screenMode == ScreenMode::Options)
	{
		DrawOptions(&memDc);
		PresentFrame();
		return;
	}
	if (_screenMode == ScreenMode::Statistics)
	{
		DrawStatisticsScreen(&memDc);
		PresentFrame();
		return;
	}
	if (_screenMode == ScreenMode::Reward)
	{
		DrawRewardScreen(&memDc);
		PresentFrame();
		return;
	}
	if (_screenMode == ScreenMode::Shop)
	{
		DrawShopScreen(&memDc);
		PresentFrame();
		return;
	}
	if (_screenMode == ScreenMode::Result)
	{
		DrawResultScreen(&memDc);
		PresentFrame();
		return;
	}

	UiRenderer::DrawPanel(
		&memDc,
		CRect(
			static_cast<int>(std::lround(GameLayout::BoardLeft)),
			static_cast<int>(std::lround(GameLayout::BoardTop)),
			static_cast<int>(std::lround(GameLayout::BoardRight)),
			static_cast<int>(std::lround(GameLayout::BoardBottom))),
		false,
		UiTheme::Border);

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
	DrawAttackAnimations(&memDc);

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

	CString playerHealth;
	playerHealth.Format(
		_T("%d / %d"),
		static_cast<int>(std::lround(_game.GetPlayer().GetHp())),
		static_cast<int>(std::lround(_game.GetStage().rules.playerHealth)));
	UiRenderer::DrawText(
		&memDc,
		CRect(86, 42, 254, 66),
		_T("PLAYER"),
		90,
		UiTheme::MutedText);
	UiRenderer::DrawProgressBar(
		&memDc,
		CRect(86, 67, 254, 92),
		_game.GetStage().rules.playerHealth <= 0.0f
			? 0.0f
			: _game.GetPlayer().GetHp() / _game.GetStage().rules.playerHealth,
		playerHealth,
		UiTheme::Green,
		UiTheme::Gold);
	CString runStatus;
	runStatus.Format(
		_T("G %d  ·  NODE %zu/%zu"),
		_run.GetGold(),
		(std::min)(_run.GetClearedStageCount() + 1, _run.GetStageCount()),
		_run.GetStageCount());
	UiRenderer::DrawText(
		&memDc,
		CRect(82, 94, 258, 116),
		runStatus,
		78,
		UiTheme::Gold);

	if (_game.GetEnemy().GetHp() > 0.0f)
	{
		const EnemyCombatant& activeEnemy = enemyRoster[_game.GetActiveEnemyIndex()];
		CString targetTitle;
		targetTitle.Format(
			_T("TARGET  ·  %s  ·  %zu 남음"),
			Utf8Text(_game.GetActiveEnemyDefinition().displayName).GetString(),
			_game.GetLivingEnemyCount());
		CString enemyAction;
		enemyAction.Format(
			_T("거리 %d  ·  사거리 %d  ·  "),
			activeEnemy.distanceToPlayerCells,
			activeEnemy.definition.attackRangeCells);
		enemyAction += EnemyActionText(_game.GetNextEnemyAction());
		if (_game.GetEnemyShield() > 0.0f)
		{
			CString shieldText;
			shieldText.Format(
				_T(" · 방어막 %d"),
				static_cast<int>(std::lround(_game.GetEnemyShield())));
			enemyAction += shieldText;
		}
		const CRect targetPanel(640, 39, 970, 110);
		UiRenderer::DrawPanel(&memDc, targetPanel, false, UiTheme::Orange);
		UiRenderer::DrawText(
			&memDc,
			CRect(650, 45, 960, 70),
			targetTitle,
			88,
			UiTheme::Gold);
		UiRenderer::DrawText(
			&memDc,
			CRect(650, 72, 960, 103),
			enemyAction,
			80,
			activeEnemy.IsPlayerInRange() ? UiTheme::Danger : UiTheme::Text);
	}

	if (_options.showGameplayInfo)
	{
		const CRect infoPanel(280, 39, 620, 116);
		UiRenderer::DrawPanel(&memDc, infoPanel, false, UiTheme::Blue);
		UiRenderer::DrawText(
			&memDc,
			CRect(292, 44, 608, 67),
			StateText(_game.GetState()),
			83,
			UiTheme::Blue);
		UiRenderer::DrawText(
			&memDc,
			CRect(290, 68, 610, 91),
			FeedbackText(_game.GetFeedback(), _game.GetScore()),
			76,
			UiTheme::Text);
		CString optionsText;
		optionsText.Format(
			_T("%s  ·  %s(M)  ·  %s"),
			DifficultyTextForUi(_game.GetDifficulty()).GetString(),
			_options.soundEnabled ? _T("소리") : _T("음소거"),
			_options.pegColorMode == PegColorMode::HighContrast ? _T("고대비") : _T("표준"));
		UiRenderer::DrawText(
			&memDc,
			CRect(290, 92, 610, 111),
			optionsText,
			70,
			UiTheme::MutedText);
	}
	DrawPlayingLoadout(&memDc);
	DrawGameplayTooltip(&memDc);
	DrawCombatLog(&memDc);

	DrawFeedbackAnimations(&memDc);

	PresentFrame();
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
	_enemyWolfSprite.LoadBitmap(IDB_ENEMY_THORNBACK_WOLF_V1);
	_enemyWispSprite.LoadBitmap(IDB_ENEMY_AZURE_WISP_V1);
	_orbSprite.LoadBitmap(IDB_ORB_AMBER_TEAL_V2);
	_orbTravelerIcon.LoadBitmap(IDB_ORB_TRAVELER_V1);
	_orbIronIcon.LoadBitmap(IDB_ORB_IRON_V1);
	_orbEchoIcon.LoadBitmap(IDB_ORB_ECHO_V1);
	_relicComboLanternIcon.LoadBitmap(IDB_RELIC_COMBO_LANTERN_V1);
	_relicThornCharmIcon.LoadBitmap(IDB_RELIC_THORN_CHARM_V1);
	_relicBarkGuardIcon.LoadBitmap(IDB_RELIC_BARK_GUARD_V1);
	_shopMerchantSprite.LoadBitmap(IDB_SHOP_MERCHANT_V1);
	_stagePreviewForest.LoadBitmap(IDB_STAGE_PREVIEW_FOREST_V1);
	_stagePreviewCrystal.LoadBitmap(IDB_STAGE_PREVIEW_CRYSTAL_V1);
	_stagePreviewFungal.LoadBitmap(IDB_STAGE_PREVIEW_FUNGAL_V1);
	_stagePreviewEmber.LoadBitmap(IDB_STAGE_PREVIEW_EMBER_V1);
	_stagePreviewCitadel.LoadBitmap(IDB_STAGE_PREVIEW_CITADEL_V1);

	CWnd* mainWindow = AfxGetMainWnd();
	CMenu* mainMenu = mainWindow != nullptr ? mainWindow->GetMenu() : nullptr;
	if (mainMenu != nullptr && mainMenu->GetMenuItemCount() > 0)
	{
		CMenu* gameOptionsMenu = mainMenu->GetSubMenu(mainMenu->GetMenuItemCount() - 1);
		if (gameOptionsMenu != nullptr)
		{
			gameOptionsMenu->AppendMenu(MF_SEPARATOR);
			gameOptionsMenu->AppendMenu(MF_STRING, ID_GAMEPLAY_INFO, _T("인게임 정보 표시"));
			mainWindow->DrawMenuBar();
		}
	}

	constexpr UINT GAME_TIMER_INTERVAL_MS = 10;
	_gameTimerId = SetTimer(1, GAME_TIMER_INTERVAL_MS, nullptr);
	if (_gameTimerId == 0)
	{
		return -1;
	}

	_lastFrameTime = std::chrono::steady_clock::now();
	_accumulatedTimeSeconds = 0.0;
	UpdateScreenMusic();
	if (_demoRequested)
	{
		SetDemoMode(true);
	}

	return 0;
}

void CChildView::OnDestroy()
{
	ReleaseMouseInput(true);
	_audioPlayer.StopAll();
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
	if (_enemyWolfSprite.GetSafeHandle() != nullptr)
	{
		_enemyWolfSprite.DeleteObject();
	}
	if (_enemyWispSprite.GetSafeHandle() != nullptr)
	{
		_enemyWispSprite.DeleteObject();
	}
	if (_orbSprite.GetSafeHandle() != nullptr)
	{
		_orbSprite.DeleteObject();
	}
	for (CBitmap* itemIcon : {
		&_orbTravelerIcon,
		&_orbIronIcon,
		&_orbEchoIcon,
		&_relicComboLanternIcon,
		&_relicThornCharmIcon,
		&_relicBarkGuardIcon,
		&_shopMerchantSprite,
		&_stagePreviewForest,
		&_stagePreviewCrystal,
		&_stagePreviewFungal,
		&_stagePreviewEmber,
		&_stagePreviewCitadel })
	{
		if (itemIcon->GetSafeHandle() != nullptr)
		{
			itemIcon->DeleteObject();
		}
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
	_audioPlayer.Update(deltaSeconds);
	PollGamepad();
	UpdateDemoRun(deltaSeconds);
	_screenTransition.Update(deltaSeconds);
	_damageFlashSeconds = (std::max)(0.0f, _damageFlashSeconds - deltaSeconds);
	if (_resetConfirmation != ResetConfirmation::None)
	{
		_resetConfirmationSeconds = (std::max)(0.0f, _resetConfirmationSeconds - deltaSeconds);
		if (_resetConfirmationSeconds <= 0.0f)
		{
			_resetConfirmation = ResetConfirmation::None;
			_optionsNotice = _T("초기화 확인 시간이 만료되었습니다");
		}
	}
	if (_screenMode != ScreenMode::Playing)
	{
		UpdateFeedbackAnimations(deltaSeconds);
		UpdateAttackAnimations(deltaSeconds);
		if (_screenMode == ScreenMode::Reward && _acquiredReward.has_value())
		{
			_rewardAcquisitionSeconds += deltaSeconds;
			if (_rewardAcquisitionSeconds >= 1.25f)
			{
				_acquiredReward.reset();
				_rewardAcquisitionSeconds = 0.0f;
				SetScreenMode(ScreenMode::StageSelection);
			}
		}
		return;
	}
	if (_terminalTransition.IsPending())
	{
		UpdateFeedbackAnimations(deltaSeconds);
		UpdateAttackAnimations(deltaSeconds);
		UpdateOrbVisuals(deltaSeconds);
		FinishPendingTerminalTransition();
		return;
	}

	const GameUpdateResult result = _game.Update(deltaSeconds);
	ConsumeGameEvents();
	UpdateFeedbackAnimations(deltaSeconds);
	UpdateAttackAnimations(deltaSeconds);
	UpdateOrbVisuals(deltaSeconds);

	_terminalTransition.Queue(result);
	FinishPendingTerminalTransition();
}

void CChildView::SetDemoMode(bool enabled)
{
	_demoRequested = false;
	_demoRun.SetEnabled(enabled);
	if (enabled && _screenMode != ScreenMode::Playing)
	{
		BeginNewRun();
		StartSelectedStage();
	}
	Invalidate(FALSE);
}

void CChildView::UpdateDemoRun(float deltaSeconds)
{
	if (!_demoRun.IsEnabled() || _screenMode != ScreenMode::Playing)
	{
		return;
	}
	const std::optional<DemoAction> action = _demoRun.Update(deltaSeconds, _game.GetState());
	if (!action.has_value())
	{
		return;
	}

	const Vector2 ballPosition = _game.GetBall().GetPosition();
	const Vector2 direction = action->direction.Normalized();
	const Vector2 dragPosition = ballPosition - direction * action->dragDistance;
	if (action->type == DemoActionType::BeginAim)
	{
		if (_game.BeginAim(ballPosition)) _game.UpdateAim(dragPosition);
	}
	else if (_game.GetBall().GetClick())
	{
		_game.ReleaseShot(dragPosition);
	}
}

void CChildView::DrawDemoBadge(CDC* deviceContext)
{
	if (!_demoRun.IsEnabled()) return;
	const CRect badge(852, 18, 968, 50);
	UiRenderer::DrawPanel(deviceContext, badge, true, UiTheme::Orange);
	UiRenderer::DrawText(deviceContext, badge, _T("DEMO · F9"), 88, UiTheme::Orange);
}

void CChildView::FinishPendingTerminalTransition()
{
	const GameUpdateResult result = _terminalTransition.CompleteIfReady(
		_attackAnimations.empty(),
		_feedbackAnimations.empty());
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
	std::size_t queuedToastCount = static_cast<std::size_t>(std::count_if(
		_feedbackAnimations.begin(),
		_feedbackAnimations.end(),
		[](const FeedbackAnimation& animation) { return animation.toast; }));
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
		case GameEventType::RefreshGuaranteed:
			animation.text = _T("REFRESH READY");
			animation.color = RGB(0, 210, 120);
			animation.lifetimeSeconds = 1.1f;
			animation.toast = true;
			break;
		case GameEventType::RefreshRelocated:
			if (event.affectedPegs > 1)
			{
				animation.text.Format(_T("REFRESH MOVED x%d"), event.affectedPegs);
			}
			else
			{
				animation.text = _T("REFRESH MOVED");
			}
			animation.color = RGB(0, 225, 155);
			animation.lifetimeSeconds = 1.0f;
			animation.toast = true;
			break;
		case GameEventType::PlayerAttack:
		{
			const CString deliveryText = event.attackDelivery == AttackDelivery::Melee
				? _T("MELEE")
				: _T("PROJECTILE");
			if (event.attackTarget == AttackTarget::All)
			{
				animation.text.Format(_T("%s · ALL x%d"), deliveryText.GetString(), event.affectedPegs);
			}
			else
			{
				animation.text.Format(_T("%s · SINGLE"), deliveryText.GetString());
			}
			animation.position = { 490.0f, 220.0f };
			animation.color = event.attackTarget == AttackTarget::All
				? RGB(190, 110, 245)
				: RGB(255, 194, 62);
			animation.lifetimeSeconds = 1.0f;
			animation.toast = true;

			const auto& enemies = _game.GetEnemies();
			const Vector2 start = GameLayout::PlayerPosition
				+ Vector2{ GameLayout::PlayerSize.x - 8.0f, GameLayout::PlayerSize.y };
			for (std::size_t index = 0; index < enemies.size(); ++index)
			{
				if (event.attackTarget == AttackTarget::Single && index != event.targetEnemyIndex)
				{
					continue;
				}
				const bool enemyGroup = enemies.size() > 1;
				const Vector2 enemySize = enemyGroup ? GameLayout::EnemyGroupSize : GameLayout::EnemySize;
				const float enemyY = enemyGroup ? GameLayout::EnemyGroupY : GameLayout::EnemyInitialPosition.y;
				_attackAnimations.push_back({
					event.attackDelivery,
					event.attackTarget,
					start,
					{ enemies[index].actor.GetX() + enemySize.x * 0.5f, enemyY + enemySize.y * 0.65f },
					event.attackTarget == AttackTarget::All ? RGB(190, 110, 245) : RGB(255, 194, 62),
					0.0f,
					event.attackDelivery == AttackDelivery::Melee ? 0.45f : 0.65f
				});
			}
			break;
		}
		case GameEventType::TurnResolved:
			animation.text.Format(_T("TURN +%d"), event.scoreAwarded);
			animation.position = GameLayout::TurnEffectPosition;
			animation.color = RGB(40, 100, 220);
			animation.toast = true;
			break;
		case GameEventType::EnemyAdvanced:
			animation.text.Format(_T("1칸 전진 · 거리 %d칸"), event.affectedPegs);
			animation.color = RGB(240, 180, 80);
			animation.toast = true;
			break;
		case GameEventType::EnemyFortified:
			animation.text.Format(_T("SHIELD +%d"), static_cast<int>(std::lround(event.damage)));
			animation.color = RGB(90, 180, 255);
			animation.lifetimeSeconds = 1.1f;
			animation.toast = true;
			break;
		case GameEventType::EnemyDefeated:
			animation.text.Format(_T("DEFEATED · %d LEFT"), event.affectedPegs);
			animation.color = RGB(255, 194, 62);
			animation.lifetimeSeconds = 1.25f;
			animation.toast = true;
			break;
		case GameEventType::PlayerDamaged:
			animation.text.Format(_T("HP -%d"), static_cast<int>(event.damage));
			animation.color = RGB(220, 0, 0);
			animation.lifetimeSeconds = 1.2f;
			animation.toast = true;
			_damageFlashSeconds = 0.45f;
			if (event.targetEnemyIndex < _game.GetEnemies().size())
			{
				const bool enemyGroup = _game.GetEnemies().size() > 1;
				const Vector2 enemySize = enemyGroup
					? GameLayout::EnemyGroupSize
					: GameLayout::EnemySize;
				const float enemyY = enemyGroup
					? GameLayout::EnemyGroupY
					: GameLayout::EnemyInitialPosition.y;
				const EnemyCombatant& attacker = _game.GetEnemies()[event.targetEnemyIndex];
				_attackAnimations.push_back({
					event.attackDelivery,
					AttackTarget::Single,
					{ attacker.actor.GetX() + enemySize.x * 0.5f, enemyY + enemySize.y * 0.65f },
					GameLayout::PlayerPosition + Vector2{ GameLayout::PlayerSize.x * 0.55f, GameLayout::PlayerSize.y * 0.65f },
					RGB(220, 70, 60),
					0.0f,
					event.attackDelivery == AttackDelivery::Melee ? 0.45f : 0.65f
				});
			}
			break;
		case GameEventType::Victory:
			animation.text = _T("CLEAR!");
			animation.position = GameLayout::TurnEffectPosition;
			animation.color = RGB(0, 150, 60);
			animation.toast = true;
			break;
		case GameEventType::Defeat:
			animation.text = _T("GAME OVER");
			animation.position = GameLayout::TurnEffectPosition;
			animation.color = RGB(220, 0, 0);
			animation.toast = true;
			break;
		}

		if (animation.toast)
		{
			animation.position = { 500.0f, 275.0f };
			animation.ageSeconds = -0.34f * static_cast<float>(queuedToastCount++);
		}
		_feedbackAnimations.push_back(std::move(animation));
		if (IsCombatLogEvent(event.type))
		{
			_combatLog.Add(
				std::wstring(_feedbackAnimations.back().text.GetString()),
				CombatTone(event.type));
		}
		PlayEventSound(event.type, event.pegType);
	}
}

void CChildView::UpdateAttackAnimations(float deltaSeconds)
{
	for (AttackAnimation& animation : _attackAnimations)
	{
		animation.ageSeconds += deltaSeconds;
	}
	_attackAnimations.erase(
		std::remove_if(
			_attackAnimations.begin(),
			_attackAnimations.end(),
			[](const AttackAnimation& animation)
			{
				return animation.ageSeconds >= animation.lifetimeSeconds;
			}),
		_attackAnimations.end());
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
		if (animation.ageSeconds < 0.0f)
		{
			continue;
		}
		const float progress = animation.ageSeconds / animation.lifetimeSeconds;
		deviceContext->SetTextColor(animation.color);
		if (animation.toast)
		{
			const int lift = static_cast<int>(std::lround(progress * 8.0f));
			const CRect toastBounds(330, 250 - lift, 670, 294 - lift);
			UiRenderer::DrawPanel(deviceContext, toastBounds, true, animation.color);
			UiRenderer::DrawText(deviceContext, toastBounds, animation.text, 115, animation.color);
			continue;
		}
		deviceContext->TextOut(
			static_cast<int>(std::lround(animation.position.x - 30.0f)),
			static_cast<int>(std::lround(animation.position.y - progress * 45.0f)),
			animation.text);
	}

	deviceContext->RestoreDC(savedDc);
}

void CChildView::DrawUiAnimations(CDC* deviceContext, const CRect& clientBounds)
{
	if (_damageFlashSeconds > 0.0f)
	{
		const float intensity = std::clamp(_damageFlashSeconds / 0.45f, 0.0f, 1.0f);
		const int thickness = 3 + static_cast<int>(std::lround(intensity * 9.0f));
		const int savedDc = deviceContext->SaveDC();
		CPen damagePen(PS_SOLID, thickness, RGB(220, 45, 45));
		deviceContext->SelectObject(&damagePen);
		deviceContext->SelectStockObject(NULL_BRUSH);
		CRect damageBounds(clientBounds);
		damageBounds.DeflateRect(thickness, thickness);
		deviceContext->Rectangle(damageBounds);
		deviceContext->RestoreDC(savedDc);
	}

	if (!_screenTransition.IsActive())
	{
		return;
	}
	const float revealed = _screenTransition.EaseOutProgress();
	const int coverHeight = static_cast<int>(std::lround(
		static_cast<float>(clientBounds.Height()) * 0.5f * (1.0f - revealed)));
	if (coverHeight <= 0)
	{
		return;
	}
	deviceContext->FillSolidRect(
		CRect(clientBounds.left, clientBounds.top, clientBounds.right, clientBounds.top + coverHeight),
		UiTheme::Canvas);
	deviceContext->FillSolidRect(
		CRect(clientBounds.left, clientBounds.bottom - coverHeight, clientBounds.right, clientBounds.bottom),
		UiTheme::Canvas);
}

void CChildView::DrawGamepadFocus(CDC* deviceContext)
{
	if (!_gamepadConnected || _screenMode == ScreenMode::Playing)
	{
		return;
	}
	const UiFocusRect focus = GetGamepadFocusRect(
		CurrentUiScreen(),
		_gamepadFocusIndex,
		VisibleStageCount());
	if (!focus.IsValid())
	{
		return;
	}
	const int savedDc = deviceContext->SaveDC();
	CPen focusPen(PS_SOLID, 4, UiTheme::Gold);
	deviceContext->SelectObject(&focusPen);
	deviceContext->SelectStockObject(NULL_BRUSH);
	CRect bounds(
		static_cast<int>(std::lround(focus.left)),
		static_cast<int>(std::lround(focus.top)),
		static_cast<int>(std::lround(focus.right)),
		static_cast<int>(std::lround(focus.bottom)));
	bounds.InflateRect(4, 4);
	deviceContext->RoundRect(bounds, CPoint(18, 18));
	deviceContext->RestoreDC(savedDc);
	UiRenderer::DrawText(
		deviceContext,
		CRect(bounds.right - 46, bounds.top - 2, bounds.right - 4, bounds.top + 28),
		_T("[A]"),
		85,
		UiTheme::Gold);
}

void CChildView::DrawAttackAnimations(CDC* deviceContext)
{
	const int savedDc = deviceContext->SaveDC();
	deviceContext->SetBkMode(TRANSPARENT);
	for (const AttackAnimation& animation : _attackAnimations)
	{
		const float progress = std::clamp(
			animation.ageSeconds / animation.lifetimeSeconds,
			0.0f,
			1.0f);
		CPen attackPen(PS_SOLID, animation.delivery == AttackDelivery::Melee ? 6 : 3, animation.color);
		deviceContext->SelectObject(&attackPen);
		if (animation.delivery == AttackDelivery::Projectile)
		{
			const Vector2 projectile = animation.start
				+ (animation.end - animation.start) * progress;
			const Vector2 tail = projectile
				- (animation.end - animation.start).Normalized() * 22.0f;
			deviceContext->MoveTo(
				static_cast<int>(std::lround(tail.x)),
				static_cast<int>(std::lround(tail.y)));
			deviceContext->LineTo(
				static_cast<int>(std::lround(projectile.x)),
				static_cast<int>(std::lround(projectile.y)));
			CBrush projectileBrush(animation.color);
			deviceContext->SelectObject(&projectileBrush);
			deviceContext->SelectStockObject(NULL_PEN);
			const int x = static_cast<int>(std::lround(projectile.x));
			const int y = static_cast<int>(std::lround(projectile.y));
			deviceContext->Ellipse(x - 8, y - 8, x + 8, y + 8);
		}
		else
		{
			const float spread = 12.0f + progress * 24.0f;
			const int centerX = static_cast<int>(std::lround(animation.end.x));
			const int centerY = static_cast<int>(std::lround(animation.end.y));
			const int radius = static_cast<int>(std::lround(spread));
			deviceContext->MoveTo(centerX - radius, centerY + radius);
			deviceContext->LineTo(centerX + radius, centerY - radius);
			deviceContext->MoveTo(centerX - radius / 2, centerY - radius);
			deviceContext->LineTo(centerX + radius, centerY + radius / 2);
		}
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

	const int labelTop = static_cast<int>(std::lround(drawY + drawSize.y
		+ (enemyGroup ? 3.0f : GameLayout::EnemyHealthBarHeight + 8.0f)));
	CString rangeText;
	rangeText.Format(
		_T("거리%d / 범위%d"),
		combatant.distanceToPlayerCells,
		combatant.definition.attackRangeCells);
	const int textState = deviceContext->SaveDC();
	deviceContext->SetBkMode(TRANSPARENT);
	deviceContext->SetTextColor(
		combatant.IsPlayerInRange() ? UiTheme::Danger : RGB(226, 214, 184));
	deviceContext->DrawText(
		rangeText,
		CRect(left - 8, labelTop, right + 8, labelTop + 18),
		DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	deviceContext->RestoreDC(textState);
}

void CChildView::DrawStageSelection(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	DrawMenuTitle(deviceContext, Text("screen.stage_selection"));

	const CRect statusPanel(24, 105, 225, 700);
	UiRenderer::DrawPanel(deviceContext, statusPanel);
	UiRenderer::DrawText(deviceContext, CRect(40, 122, 209, 164), _T("ADVENTURE"), 150, UiTheme::Gold);
	CString runProgress;
	runProgress.Format(
		_T("FLOOR  %zu / %zu"),
		(std::min)(_run.GetClearedStageCount() + 1, _run.GetStageCount()),
		_run.GetStageCount());
	UiRenderer::DrawText(deviceContext, CRect(42, 178, 207, 208), runProgress, 105, UiTheme::Green);
	const float displayedHealth = _runPlayerHealth > 0.0f
		? _runPlayerHealth
		: _game.GetPlayer().GetHp();
	const float maximumHealth = _game.GetStage().rules.playerHealth > 0.0f
		? _game.GetStage().rules.playerHealth
		: 100.0f;
	CString healthText;
	healthText.Format(
		_T("HP  %d / %d"),
		static_cast<int>(std::lround(displayedHealth)),
		static_cast<int>(std::lround(maximumHealth)));
	UiRenderer::DrawProgressBar(
		deviceContext,
		CRect(43, 216, 206, 242),
		displayedHealth / maximumHealth,
		healthText,
		UiTheme::Green,
		UiTheme::Gold);
	CString goldText;
	goldText.Format(_T("GOLD  %d"), _run.GetGold());
	UiRenderer::DrawPanel(deviceContext, CRect(43, 254, 206, 296), false, UiTheme::Gold);
	UiRenderer::DrawText(deviceContext, CRect(50, 258, 199, 292), goldText, 110, UiTheme::Gold);

	UiRenderer::DrawText(deviceContext, CRect(42, 315, 207, 342), _T("CURRENT ORB"), 90, UiTheme::MutedText);
	UiRenderer::DrawPanel(deviceContext, CRect(43, 346, 206, 432), false, UiTheme::Blue);
	DrawOrbIcon(
		deviceContext,
		CRect(52, 361, 108, 417),
		_game.GetLoadout().GetSelectedOrb());
	UiRenderer::DrawText(
		deviceContext,
		CRect(112, 357, 199, 421),
		Utf8Text(_game.GetLoadout().GetSelectedOrb().displayName),
		95,
		UiTheme::Text,
		DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawText(deviceContext, CRect(42, 449, 207, 476), _T("RELICS"), 90, UiTheme::MutedText);
	UiRenderer::DrawText(
		deviceContext,
		CRect(43, 478, 206, 555),
		RelicSummary(_game.GetLoadout()),
		82,
		UiTheme::Green,
		DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawKeyHint(deviceContext, CRect(43, 565, 206, 602), _T("[T] 통계"));
	UiRenderer::DrawKeyHint(deviceContext, CRect(43, 608, 206, 646), Text("hint.loadout"));
	UiRenderer::DrawKeyHint(deviceContext, CRect(43, 652, 206, 690), Text("hint.options"));

	std::vector<std::string> visibleStageIds;
	if (_run.GetStatus() == RunStatus::StageChoice)
	{
		visibleStageIds = _run.GetAvailableStageIds();
	}
	else if (!_run.GetCurrentStageId().empty())
	{
		visibleStageIds.push_back(_run.GetCurrentStageId());
	}
	const std::size_t visibleStageCount = (std::min)(std::size_t{ 2 }, visibleStageIds.size());
	const std::optional<std::size_t> selectedChoiceIndex =
		_run.GetSelectedStageChoiceIndex();
	const std::size_t stageCount = _run.GetStageCount();
	const std::size_t clearedStageCount = _run.GetClearedStageCount();
	const CRect routePanel(252, 92, 965, 192);
	UiRenderer::DrawPanel(deviceContext, routePanel, false, UiTheme::Gold);
	UiRenderer::DrawText(deviceContext, CRect(270, 101, 947, 124), _T("RUN ROUTE"), 90, UiTheme::MutedText);
	if (stageCount > 0)
	{
		constexpr int ROUTE_LEFT = 287;
		constexpr int ROUTE_RIGHT = 930;
		constexpr int ROUTE_Y = 149;
		const int savedDc = deviceContext->SaveDC();
		CPen routePen(PS_SOLID, 4, UiTheme::Border);
		deviceContext->SelectObject(&routePen);
		deviceContext->MoveTo(ROUTE_LEFT, ROUTE_Y);
		deviceContext->LineTo(ROUTE_RIGHT, ROUTE_Y);
		for (std::size_t index = 0; index < stageCount; ++index)
		{
			const float fraction = stageCount == 1
				? 0.5f
				: static_cast<float>(index) / static_cast<float>(stageCount - 1);
			const int centerX = ROUTE_LEFT + static_cast<int>(std::lround(
				static_cast<float>(ROUTE_RIGHT - ROUTE_LEFT) * fraction));
			const bool completed = index < clearedStageCount;
			const bool current = index == clearedStageCount;
			const COLORREF nodeColor = completed
				? UiTheme::Green
				: (current ? UiTheme::Gold : UiTheme::Border);
			const int nodeDc = deviceContext->SaveDC();
			CPen nodePen(PS_SOLID, current ? 4 : 2, nodeColor);
			CBrush nodeBrush(completed ? UiTheme::Green : UiTheme::PanelMuted);
			deviceContext->SelectObject(&nodePen);
			deviceContext->SelectObject(&nodeBrush);
			const int radius = current ? 13 : 10;
			deviceContext->Ellipse(
				centerX - radius,
				ROUTE_Y - radius,
				centerX + radius,
				ROUTE_Y + radius);
			deviceContext->RestoreDC(nodeDc);

			CString nodeText;
			nodeText.Format(_T("%zu"), index + 1);
			UiRenderer::DrawText(
				deviceContext,
				CRect(centerX - 18, ROUTE_Y + 17, centerX + 18, ROUTE_Y + 37),
				nodeText,
				70,
				nodeColor);
		}
		deviceContext->RestoreDC(savedDc);
	}

	for (std::size_t index = 0; index < visibleStageCount; ++index)
	{
		const bool shopStage = IsRunShopStage(visibleStageIds[index]);
		const StageDefinition* source = FindContentStage(
			_contentCatalog.stages,
			visibleStageIds[index]);
		if (source == nullptr && !shopStage)
		{
			continue;
		}
		const int left = visibleStageCount == 1
			? 437
			: (index == 0 ? 252 : 620);
		const CRect card(left, 215, left + 343, 590);
		const bool selected = _run.GetStatus() == RunStatus::StageReady
			|| (_run.GetStatus() == RunStatus::StageChoice
				&& selectedChoiceIndex.has_value()
				&& *selectedChoiceIndex == index);
		const COLORREF stageColor = shopStage
			? UiTheme::Green
			: (source->isBoss ? UiTheme::Orange : UiTheme::Gold);
		UiRenderer::DrawPanel(deviceContext, card, selected, stageColor);
		const CRect imageBounds(card.left + 12, card.top + 12, card.right - 12, card.top + 252);
		if (shopStage)
		{
			deviceContext->FillSolidRect(imageBounds, RGB(14, 28, 27));
			UiRenderer::DrawTransparentBitmap(
				deviceContext,
				&_shopMerchantSprite,
				CRect(imageBounds.left + 70, imageBounds.top + 10, imageBounds.right - 70, imageBounds.bottom - 10));
		}
		else
		{
			UiRenderer::DrawBackdrop(deviceContext, GetStagePreview(visibleStageIds[index]), imageBounds);
		}
		const int imageState = deviceContext->SaveDC();
		CPen imageBorder(PS_SOLID, selected ? 3 : 1, selected ? stageColor : UiTheme::Border);
		deviceContext->SelectObject(&imageBorder);
		deviceContext->SelectStockObject(NULL_BRUSH);
		deviceContext->Rectangle(imageBounds);
		deviceContext->RestoreDC(imageState);

		CString title;
		title.Format(
			_T("[%zu] %s"),
			index + 1,
			shopStage ? _T("Goblin Market") : Utf8Text(source->displayName).GetString());
		UiRenderer::DrawText(
			deviceContext,
			CRect(card.left + 14, card.top + 263, card.right - 14, card.top + 310),
			title,
			155,
			UiTheme::Text);
		const CString typeText = shopStage
			? CString(_T("SHOP"))
			: (source->isBoss ? CString(_T("BOSS")) : CString(_T("BATTLE")));
		UiRenderer::DrawText(
			deviceContext,
			CRect(card.left + 14, card.top + 310, card.right - 14, card.top + 342),
			typeText,
			95,
			stageColor);
		UiRenderer::DrawText(
			deviceContext,
			CRect(card.left + 14, card.top + 343, card.right - 14, card.bottom - 8),
			selected ? CString(_T("선택됨")) : CString(_T("클릭하여 선택")),
			90,
			selected ? stageColor : UiTheme::MutedText);
	}

	UiRenderer::DrawKeyHint(
		deviceContext,
		CRect(350, 635, 865, 695),
		_run.GetStatus() != RunStatus::StageChoice
			? Text("hint.start")
			: (selectedChoiceIndex.has_value()
				? CString(_T("선택 변경 가능 · ENTER 또는 클릭으로 출발"))
				: CString(_T("두 경로 중 하나를 선택하세요"))));
	if (!_contentCatalog.UsedExternalContent() || !_gameplayCatalog.UsedExternalContent())
	{
		UiRenderer::DrawText(deviceContext, CRect(250, 600, 965, 628), _T("외부 콘텐츠 오류 · 검증된 내장 카탈로그 사용 중"), 90, UiTheme::Orange);
	}
	else if (!_localization.UsedExternalContent() || _localization.fallbackKeyCount > 0)
	{
		UiRenderer::DrawText(deviceContext, CRect(250, 600, 965, 628), Text("notice.external_fallback"), 90, UiTheme::Orange);
	}
}

void CChildView::DrawRewardScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	auto DrawRewardIcon = [this, deviceContext](const RunReward& reward, const CRect& bounds)
	{
		if (reward.kind == RunRewardKind::Orb)
		{
			const OrbDefinition* orb = FindOrbDefinition(reward.id);
			if (orb != nullptr)
			{
				DrawOrbIcon(deviceContext, bounds, *orb);
				return;
			}
		}
		else if (reward.kind == RunRewardKind::Relic)
		{
			const RelicDefinition* relic = FindRelicDefinition(reward.id);
			if (relic != nullptr)
			{
				DrawRelicIcon(deviceContext, bounds, *relic);
				return;
			}
		}
		DrawHealIcon(deviceContext, bounds);
	};
	CString rewardHealth;
	rewardHealth.Format(
		_T("HP  %d / %d"),
		static_cast<int>(std::lround(_runPlayerHealth)),
		static_cast<int>(std::lround(_game.GetStage().rules.playerHealth)));
	UiRenderer::DrawProgressBar(
		deviceContext,
		CRect(55, 96, 235, 122),
		_game.GetStage().rules.playerHealth <= 0.0f
			? 0.0f
			: _runPlayerHealth / _game.GetStage().rules.playerHealth,
		rewardHealth,
		UiTheme::Green,
		UiTheme::Gold);
	CString rewardGold;
	rewardGold.Format(_T("GOLD  %d"), _run.GetGold());
	UiRenderer::DrawPanel(deviceContext, CRect(765, 91, 945, 127), false, UiTheme::Gold);
	UiRenderer::DrawText(deviceContext, CRect(775, 95, 935, 123), rewardGold, 100, UiTheme::Gold);

	if (_acquiredReward.has_value())
	{
		const RunReward& acquired = *_acquiredReward;
		const COLORREF color = acquired.kind == RunRewardKind::Orb
			? UiTheme::Blue
			: (acquired.kind == RunRewardKind::Relic ? UiTheme::Gold : UiTheme::Green);
		DrawMenuTitle(deviceContext, _T("보상 획득"), color);
		UiRenderer::DrawText(deviceContext, CRect(250, 95, 750, 135), _T("효과 적용 후 다음 경로로 이동합니다"), 105, UiTheme::MutedText);
		const CRect acquiredCard(270, 165, 730, 550);
		UiRenderer::DrawPanel(deviceContext, acquiredCard, true, color);
		const float pulse = std::sin(std::clamp(_rewardAcquisitionSeconds / 0.8f, 0.0f, 1.0f) * 3.14159265f);
		CRect pulseBounds(acquiredCard);
		pulseBounds.InflateRect(
			6 + static_cast<int>(std::lround(pulse * 14.0f)),
			6 + static_cast<int>(std::lround(pulse * 14.0f)));
		const int savedDc = deviceContext->SaveDC();
		CPen pulsePen(PS_SOLID, 3, color);
		deviceContext->SelectObject(&pulsePen);
		deviceContext->SelectStockObject(NULL_BRUSH);
		deviceContext->RoundRect(pulseBounds, CPoint(20, 20));
		deviceContext->RestoreDC(savedDc);
		DrawRewardIcon(acquired, CRect(444, 195, 556, 307));
		UiRenderer::DrawText(deviceContext, CRect(300, 315, 700, 370), Utf8Text(acquired.displayName), 180, color);
		UiRenderer::DrawText(
			deviceContext,
			CRect(310, 380, 690, 515),
			Utf8Text(DescribeRewardEffect(acquired)),
			110,
			UiTheme::Text,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
		UiRenderer::DrawText(deviceContext, CRect(280, 575, 720, 615), _T("획득 효과 적용 완료"), 105, UiTheme::Green);
		return;
	}

	DrawMenuTitle(deviceContext, Text("screen.reward"));
	UiRenderer::DrawText(deviceContext, CRect(245, 94, 755, 132), _T("다음 전투를 위한 보상 하나를 선택하세요"), 105, UiTheme::MutedText);

	const auto& rewards = _run.GetRewardChoices();
	for (std::size_t index = 0; index < rewards.size(); ++index)
	{
		const RunReward& reward = rewards[index];
		const int left = 60 + static_cast<int>(index) * 310;
		const CRect card(left, 175, left + 280, 520);
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
		UiRenderer::DrawText(deviceContext, CRect(left + 15, 185, left + 265, 220), title, 115, color);
		DrawRewardIcon(reward, CRect(left + 96, 225, left + 184, 313));
		UiRenderer::DrawText(deviceContext, CRect(left + 12, 320, left + 268, 360), Utf8Text(reward.displayName), 125, UiTheme::Text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 14, 368, left + 266, 508),
			Utf8Text(DescribeRewardEffect(reward)),
			80,
			UiTheme::MutedText,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	}
	UiRenderer::DrawText(deviceContext, CRect(120, 535, 880, 595), _runNotice, 100, UiTheme::Orange, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawKeyHint(deviceContext, CRect(270, 630, 730, 688), _T("보상 카드 클릭 · 1/2/3 선택"));
}

void CChildView::DrawShopScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	DrawMenuTitle(deviceContext, _T("GOBLIN MARKET"), UiTheme::Green);
	CString wallet;
	wallet.Format(_T("GOLD  %d"), _run.GetGold());
	UiRenderer::DrawPanel(deviceContext, CRect(780, 91, 950, 130), false, UiTheme::Gold);
	UiRenderer::DrawText(deviceContext, CRect(790, 95, 940, 126), wallet, 110, UiTheme::Gold);

	const CRect merchantPanel(25, 145, 280, 610);
	UiRenderer::DrawPanel(deviceContext, merchantPanel, true, UiTheme::Green);
	UiRenderer::DrawTransparentBitmap(
		deviceContext,
		_shopMerchantSprite.GetSafeHandle() != nullptr ? &_shopMerchantSprite : nullptr,
		CRect(35, 165, 270, 485));
	UiRenderer::DrawText(deviceContext, CRect(45, 490, 260, 530), _T("Mosswick 상점"), 135, UiTheme::Gold);
	UiRenderer::DrawText(
		deviceContext,
		CRect(45, 530, 260, 590),
		_T("원하는 상품을 구매한 뒤\n다음 경로로 이동하세요"),
		90,
		UiTheme::MutedText,
		DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);

	const auto& offers = GetRunShopOffers();
	for (std::size_t index = 0; index < offers.size(); ++index)
	{
		const RunShopOffer& offer = offers[index];
		const int left = 290 + static_cast<int>(index) * 225;
		const CRect card(left, 165, left + 210, 535);
		const bool purchased = _shopPurchased[index];
		const bool affordable = _run.GetGold() >= offer.price;
		const COLORREF categoryColor = offer.reward.kind == RunRewardKind::Orb
			? UiTheme::Blue
			: (offer.reward.kind == RunRewardKind::Relic ? UiTheme::Gold : UiTheme::Green);
		UiRenderer::DrawPanel(
			deviceContext,
			card,
			purchased,
			purchased ? UiTheme::Green : (affordable ? categoryColor : UiTheme::MutedText));

		CString category;
		switch (offer.reward.kind)
		{
		case RunRewardKind::Orb: category = _T("ORB"); break;
		case RunRewardKind::Relic: category = _T("RELIC"); break;
		case RunRewardKind::Heal: category = _T("RECOVER"); break;
		}
		CString header;
		header.Format(_T("[%zu] %s · %d G"), index + 1, category.GetString(), offer.price);
		UiRenderer::DrawText(deviceContext, CRect(left + 8, 176, left + 202, 210), header, 95, categoryColor);

		if (offer.reward.kind == RunRewardKind::Orb)
		{
			const OrbDefinition* orb = FindOrbDefinition(offer.reward.id);
			if (orb != nullptr)
			{
				DrawOrbIcon(deviceContext, CRect(left + 66, 217, left + 144, 295), *orb);
			}
		}
		else if (offer.reward.kind == RunRewardKind::Relic)
		{
			const RelicDefinition* relic = FindRelicDefinition(offer.reward.id);
			if (relic != nullptr)
			{
				DrawRelicIcon(deviceContext, CRect(left + 66, 217, left + 144, 295), *relic);
			}
		}
		else
		{
			DrawHealIcon(deviceContext, CRect(left + 66, 217, left + 144, 295));
		}

		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 8, 302, left + 202, 340),
			Utf8Text(offer.reward.displayName),
			105,
			UiTheme::Text);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 10, 348, left + 200, 465),
			Utf8Text(DescribeRewardEffect(offer.reward)),
			70,
			UiTheme::MutedText,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 10, 485, left + 200, 523),
			purchased ? _T("구매 완료") : (affordable ? _T("구매 가능") : _T("골드 부족")),
			95,
			purchased ? UiTheme::Green : (affordable ? UiTheme::Gold : UiTheme::Danger));
	}

	UiRenderer::DrawText(deviceContext, CRect(290, 548, 950, 605), _runNotice, 95, UiTheme::Orange, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawKeyHint(deviceContext, CRect(340, 630, 660, 688), _T("상점 나가기 · ENTER / ESC"));
}

void CChildView::DrawLoadoutScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	DrawMenuTitle(deviceContext, Text("screen.loadout"));
	UiRenderer::DrawText(deviceContext, CRect(150, 88, 850, 118), _T("선택한 오브는 다음 전투의 첫 순서로 배치됩니다"), 100, UiTheme::MutedText);
	UiRenderer::DrawText(deviceContext, CRect(55, 120, 945, 145), _T("ORB BAG"), 95, UiTheme::Blue, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	const auto& orbs = GetOrbDefinitions();
	const auto& ownedOrbs = _game.GetLoadout().GetOwnedOrbs();
	for (std::size_t index = 0; index < orbs.size(); ++index)
	{
		const OrbDefinition& orb = orbs[index];
		const int left = 55 + static_cast<int>(index) * 305;
		const CRect card(left, 148, left + 280, 320);
		const bool selected = _game.GetLoadout().GetSelectedOrbId() == orb.id;
		const std::size_t ownedCount = static_cast<std::size_t>(std::count_if(
			ownedOrbs.begin(),
			ownedOrbs.end(),
			[&orb](const std::string& id) { return id == orb.id; }));
		UiRenderer::DrawPanel(deviceContext, card, selected, UiTheme::Gold);
		DrawOrbIcon(deviceContext, CRect(left + 14, 162, left + 70, 218), orb);
		CString title;
		title.Format(_T("[%zu] %s · x%zu"), index + 1, Utf8Text(orb.displayName).GetString(), ownedCount);
		UiRenderer::DrawText(deviceContext, CRect(left + 76, 160, left + 268, 220), title, 110, selected ? UiTheme::Gold : UiTheme::Text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 12, 226, left + 268, 312),
			Utf8Text(DescribeOrbEffect(orb)),
			78,
			UiTheme::MutedText,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	}

	const auto& relics = GetRelicDefinitions();
	UiRenderer::DrawText(deviceContext, CRect(55, 333, 945, 358), _T("RELICS"), 95, UiTheme::Green, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	for (std::size_t index = 0; index < relics.size(); ++index)
	{
		const RelicDefinition& relic = relics[index];
		const int left = 55 + static_cast<int>(index) * 305;
		const CRect card(left, 360, left + 280, 545);
		const std::size_t stacks = _game.GetLoadout().GetRelicStackCount(relic.id);
		const bool atLimit = stacks >= relic.maxStacks;
		UiRenderer::DrawPanel(deviceContext, card, atLimit, UiTheme::Green);
		CString title;
		title.Format(_T("[%zu] %s · %zu/%zu"), index + 4, Utf8Text(relic.displayName).GetString(), stacks, relic.maxStacks);
		DrawRelicIcon(deviceContext, CRect(left + 14, 374, left + 70, 430), relic);
		UiRenderer::DrawText(deviceContext, CRect(left + 76, 372, left + 268, 432), title, 105, atLimit ? UiTheme::Green : UiTheme::Text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 12, 438, left + 268, 535),
			Utf8Text(DescribeRelicEffect(relic)),
			72,
			UiTheme::MutedText,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	}

	const ProgressionModifiers modifiers = _game.GetProgressionModifiers();
	CString total;
	total.Format(
		_T("최종 배율 · 페그 피해 x%.2f · 점수 x%.2f · 받는 피해 x%.2f"),
		modifiers.pegDamageMultiplier,
		modifiers.scoreMultiplier,
		modifiers.incomingDamageMultiplier);
	UiRenderer::DrawText(deviceContext, CRect(140, 557, 860, 590), total, 105, UiTheme::Green);
	UiRenderer::DrawText(deviceContext, CRect(140, 592, 860, 620), _loadoutNotice, 95, UiTheme::Orange);
	UiRenderer::DrawKeyHint(deviceContext, CRect(55, 630, 300, 688), _T("초기화 · X"));
	UiRenderer::DrawText(deviceContext, CRect(305, 632, 695, 686), _T("카드 클릭 또는 1-6 키"), 95, UiTheme::MutedText);
	UiRenderer::DrawKeyHint(deviceContext, CRect(700, 630, 945, 688), _T("돌아가기 · ESC"));
}

void CChildView::DrawOptions(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	DrawMenuTitle(deviceContext, Text("screen.options"));
	const CRect optionPanel(165, 105, 835, 650);
	UiRenderer::DrawPanel(deviceContext, optionPanel);
	CString optionsGuide;
	optionsGuide.Format(
		_T("클릭하거나 단축키로 즉시 변경 · GAMEPAD %s"),
		_gamepadConnected ? _T("연결됨") : _T("미연결"));
	UiRenderer::DrawText(
		deviceContext,
		CRect(190, 122, 810, 150),
		optionsGuide,
		95,
		UiTheme::MutedText);
	CString effectsVolume;
	effectsVolume.Format(_T("%d%%"), _options.effectsVolume);
	CString musicVolume;
	musicVolume.Format(_T("%d%%"), _options.musicVolume);
	CString deadzone;
	deadzone.Format(_T("%d%%"), _options.gamepadDeadzonePercent);
	CString sensitivity;
	sensitivity.Format(_T("%d%%"), _options.gamepadSensitivityPercent);
	const CString fireBinding = _options.gamepadFireBinding == GamepadFireBinding::SouthButton
		? _T("A 버튼")
		: _T("오른쪽 트리거");
	DrawOptionTile(deviceContext, CRect(185, 145, 390, 225), _T("[D]"), Text("option.difficulty"), DifficultyTextForUi(_options.difficulty), UiTheme::Gold);
	DrawOptionTile(deviceContext, CRect(397, 145, 602, 225), _T("[M]"), Text("option.sound"), Text(_options.soundEnabled ? "value.on" : "value.off"), _options.soundEnabled ? UiTheme::Green : UiTheme::Danger);
	DrawOptionTile(deviceContext, CRect(609, 145, 815, 225), _T("[E]"), _T("효과음"), effectsVolume, UiTheme::Orange);
	DrawOptionTile(deviceContext, CRect(185, 235, 390, 315), _T("[V]"), _T("배경음"), musicVolume, UiTheme::Blue);
	DrawOptionTile(deviceContext, CRect(397, 235, 602, 315), _T("[C]"), Text("option.peg_color"), PegColorModeTextForUi(_options.pegColorMode), UiTheme::Blue);
	DrawOptionTile(deviceContext, CRect(609, 235, 815, 315), _T("[L]"), Text("option.language"), Text(_options.language == UiLanguage::Korean ? "language.korean" : "language.english"), UiTheme::Orange);
	DrawOptionTile(deviceContext, CRect(185, 325, 390, 405), _T("[Z]"), _T("패드 데드존"), deadzone, UiTheme::Green);
	DrawOptionTile(deviceContext, CRect(397, 325, 602, 405), _T("[G]"), _T("패드 감도"), sensitivity, UiTheme::Gold);
	DrawOptionTile(deviceContext, CRect(609, 325, 815, 405), _T("[F]"), _T("발사 버튼"), fireBinding, UiTheme::Orange);
	UiRenderer::DrawKeyHint(deviceContext, CRect(195, 430, 485, 478), _T("설정만 초기화 · X"));
	UiRenderer::DrawKeyHint(
		deviceContext,
		CRect(515, 430, 805, 478),
		_T("전투 기록 초기화 · R"));
	const CString audioNotice = !_audioCatalog.IsUsable()
		? CString(_T("오디오 파일을 불러오지 못해 무음으로 실행 중입니다"))
		: (_settingsSaveFailed ? Text("notice.settings_save_failed")
			: (_recordSaveFailed ? CString(_T("전투 기록을 저장하지 못했습니다"))
				: (_optionsNotice.IsEmpty() ? Text("notice.auto_save") : _optionsNotice)));
	UiRenderer::DrawText(
		deviceContext,
		CRect(190, 486, 810, 535),
		audioNotice,
		100,
		(!_audioCatalog.IsUsable() || _settingsSaveFailed || _recordSaveFailed)
			? UiTheme::Danger
			: UiTheme::Green);
	UiRenderer::DrawKeyHint(deviceContext, CRect(300, 550, 700, 612), Text("hint.back"));
}

void CChildView::DrawStatisticsScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	DrawMenuTitle(deviceContext, _T("전투 통계"), UiTheme::Green);

	CString filterText = _T("[D] 난이도: 전체");
	switch (_statisticsDifficulty)
	{
	case StatisticsDifficultyFilter::All: break;
	case StatisticsDifficultyFilter::Easy: filterText = _T("[D] 난이도: 쉬움"); break;
	case StatisticsDifficultyFilter::Normal: filterText = _T("[D] 난이도: 보통"); break;
	case StatisticsDifficultyFilter::Hard: filterText = _T("[D] 난이도: 어려움"); break;
	}
	CString sortText;
	switch (_statisticsSort)
	{
	case StatisticsSortMode::HighScore: sortText = _T("[S] 정렬: 최고 점수"); break;
	case StatisticsSortMode::ClearCount: sortText = _T("[S] 정렬: 클리어"); break;
	case StatisticsSortMode::AverageScore: sortText = _T("[S] 정렬: 평균 점수"); break;
	}
	UiRenderer::DrawKeyHint(deviceContext, CRect(80, 110, 300, 160), filterText);
	UiRenderer::DrawKeyHint(deviceContext, CRect(350, 110, 570, 160), sortText);
	CString totalText;
	totalText.Format(_T("조합 기록 %zu개"), _records.GetAllPerformance().size());
	UiRenderer::DrawText(deviceContext, CRect(620, 110, 920, 160), totalText, 100, UiTheme::Gold);

	const CRect tableBounds(45, 180, 955, 610);
	UiRenderer::DrawPanel(deviceContext, tableBounds, false, UiTheme::Green);
	const int columns[]{ 55, 300, 390, 535, 615, 695, 785, 875, 945 };
	const CString headers[]{
		_T("STAGE"), _T("난이도"), _T("ORB"), _T("PLAY"),
		_T("CLEAR"), _T("AVG"), _T("HIGH"), _T("COMBO")
	};
	for (std::size_t index = 0; index < 8U; ++index)
	{
		UiRenderer::DrawText(
			deviceContext,
			CRect(columns[index], 190, columns[index + 1], 225),
			headers[index],
			78,
			UiTheme::MutedText);
	}

	const std::vector<PerformanceRecord> rows = BuildStatisticsRows(
		_records,
		_statisticsDifficulty,
		_statisticsSort);
	if (rows.empty())
	{
		UiRenderer::DrawText(
			deviceContext,
			CRect(100, 280, 900, 480),
			_T("아직 조건에 맞는 전투 기록이 없습니다\n스테이지를 완료하거나 패배하면 자동으로 기록됩니다"),
			125,
			UiTheme::MutedText,
			DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
	}
	const std::size_t visibleRowCount = (std::min)(std::size_t{ 7 }, rows.size());
	for (std::size_t index = 0; index < visibleRowCount; ++index)
	{
		const PerformanceRecord& record = rows[index];
		const int top = 230 + static_cast<int>(index) * 52;
		if (index % 2U == 0U)
		{
			deviceContext->FillSolidRect(CRect(52, top, 948, top + 48), UiTheme::PanelMuted);
		}
		const StageDefinition* stage = FindContentStage(_contentCatalog.stages, record.stageId);
		const OrbDefinition* orb = FindOrbDefinition(record.orbId);
		CString stageName = stage != nullptr ? Utf8Text(stage->displayName) : Utf8Text(record.stageId);
		CString orbName = orb != nullptr ? Utf8Text(orb->displayName) : Utf8Text(record.orbId);
		CString values[8];
		values[0] = stageName;
		values[1] = DifficultyTextForUi(record.difficulty);
		values[2] = orbName;
		values[3].Format(_T("%d"), record.attemptCount);
		values[4].Format(_T("%d"), record.clearCount);
		values[5].Format(_T("%.0f"), record.AverageScore());
		values[6].Format(_T("%d"), record.highScore);
		values[7].Format(_T("%d"), record.bestCombo);
		for (std::size_t column = 0; column < 8U; ++column)
		{
			UiRenderer::DrawText(
				deviceContext,
				CRect(columns[column], top, columns[column + 1], top + 48),
				values[column],
				column == 0U || column == 2U ? 70 : 78,
				column >= 5U ? UiTheme::Green : UiTheme::Text,
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
		}
	}
	if (rows.size() > visibleRowCount)
	{
		CString more;
		more.Format(_T("상위 %zu개 표시 · 전체 %zu개"), visibleRowCount, rows.size());
		UiRenderer::DrawText(deviceContext, CRect(60, 570, 700, 605), more, 85, UiTheme::MutedText);
	}
	UiRenderer::DrawKeyHint(deviceContext, CRect(720, 630, 945, 688), _T("돌아가기 · B/ESC"));
}

void CChildView::DrawResultScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	const bool victory = _resultSummary.has_value()
		&& _resultSummary->result == GameUpdateResult::Victory;
	const COLORREF resultColor = victory ? UiTheme::Green : UiTheme::Danger;
	DrawMenuTitle(
		deviceContext,
		Text(victory ? "screen.run_complete" : "screen.run_failed"),
		resultColor);
	UiRenderer::DrawPanel(deviceContext, CRect(250, 120, 750, 620), true, resultColor);
	if (_resultSummary.has_value())
	{
		UiRenderer::DrawText(deviceContext, CRect(280, 155, 720, 210), Utf8Text(_resultSummary->stageName), 165, UiTheme::Gold);
		CString scoreText;
		scoreText.Format(_T("SCORE  %d"), _resultSummary->totalScore);
		UiRenderer::DrawText(deviceContext, CRect(280, 245, 720, 300), scoreText, 180);
		CString comboText;
		comboText.Format(_T("BEST COMBO  %d"), _resultSummary->bestCombo);
		UiRenderer::DrawText(deviceContext, CRect(280, 325, 720, 375), comboText, 160);
		CString turnText;
		turnText.Format(_T("TURNS  %d"), _resultSummary->turns);
		UiRenderer::DrawText(deviceContext, CRect(280, 395, 720, 445), turnText, 150);
		const StageRecord record = _records.Get(_resultSummary->stageId, _options.difficulty);
		CString recordText;
		recordText.Format(
			_T("RECORD %d · COMBO %d · CLEARS %d"),
			record.highScore,
			record.bestCombo,
			record.clearCount);
		UiRenderer::DrawText(deviceContext, CRect(280, 480, 720, 530), recordText, 115, UiTheme::Green);
	}
	if (_recordSaveFailed)
	{
		UiRenderer::DrawText(deviceContext, CRect(275, 550, 725, 595), _T("기록 저장 실패 · 현재 실행에서만 유지"), 105, UiTheme::Danger);
	}
	UiRenderer::DrawKeyHint(deviceContext, CRect(260, 635, 480, 690), Text("hint.retry"));
	UiRenderer::DrawKeyHint(deviceContext, CRect(520, 635, 740, 690), Text("hint.new_run"));
}

void CChildView::DrawMenuBackdrop(CDC* deviceContext)
{
	const CRect bounds(0, 0, 1000, 700);
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
	case EnemyVisualKind::ThornbackWolf:
		return _enemyWolfSprite.GetSafeHandle() != nullptr ? &_enemyWolfSprite : nullptr;
	case EnemyVisualKind::AzureWisp:
		return _enemyWispSprite.GetSafeHandle() != nullptr ? &_enemyWispSprite : nullptr;
	}
	return nullptr;
}

CBitmap* CChildView::GetOrbIcon(std::string_view imageKey) noexcept
{
	if (imageKey == "orb-traveler-v1")
	{
		return _orbTravelerIcon.GetSafeHandle() != nullptr ? &_orbTravelerIcon : nullptr;
	}
	if (imageKey == "orb-iron-v1")
	{
		return _orbIronIcon.GetSafeHandle() != nullptr ? &_orbIronIcon : nullptr;
	}
	if (imageKey == "orb-echo-v1")
	{
		return _orbEchoIcon.GetSafeHandle() != nullptr ? &_orbEchoIcon : nullptr;
	}
	return nullptr;
}

CBitmap* CChildView::GetRelicIcon(std::string_view imageKey) noexcept
{
	if (imageKey == "relic-combo-lantern-v1")
	{
		return _relicComboLanternIcon.GetSafeHandle() != nullptr ? &_relicComboLanternIcon : nullptr;
	}
	if (imageKey == "relic-thorn-charm-v1")
	{
		return _relicThornCharmIcon.GetSafeHandle() != nullptr ? &_relicThornCharmIcon : nullptr;
	}
	if (imageKey == "relic-bark-guard-v1")
	{
		return _relicBarkGuardIcon.GetSafeHandle() != nullptr ? &_relicBarkGuardIcon : nullptr;
	}
	return nullptr;
}

CBitmap* CChildView::GetStagePreview(std::string_view stageId) noexcept
{
	CBitmap* preview = &_stagePreviewForest;
	if (stageId == "stage-2" || stageId == "stage-6")
	{
		preview = &_stagePreviewCrystal;
	}
	else if (stageId == "stage-5" || stageId == "stage-8")
	{
		preview = &_stagePreviewFungal;
	}
	else if (stageId == "stage-7")
	{
		preview = &_stagePreviewEmber;
	}
	else if (stageId == "stage-3")
	{
		preview = &_stagePreviewCitadel;
	}
	return preview->GetSafeHandle() != nullptr ? preview : nullptr;
}

void CChildView::DrawOrbIcon(
	CDC* deviceContext,
	const CRect& bounds,
	const OrbDefinition& orb)
{
	if (!UiRenderer::DrawTransparentBitmap(
		deviceContext,
		GetOrbIcon(orb.imageKey),
		bounds))
	{
		DrawFallbackOrbIcon(deviceContext, bounds, orb.id);
	}
}

void CChildView::DrawRelicIcon(
	CDC* deviceContext,
	const CRect& bounds,
	const RelicDefinition& relic)
{
	if (!UiRenderer::DrawTransparentBitmap(
		deviceContext,
		GetRelicIcon(relic.imageKey),
		bounds))
	{
		DrawFallbackRelicIcon(deviceContext, bounds);
	}
}

void CChildView::DrawPlayingLoadout(CDC* deviceContext)
{
	const PlayerLoadout& loadout = _game.GetLoadout();
	const CRect hud(
		static_cast<int>(std::lround(GameLayout::OrbHudLeft)),
		static_cast<int>(std::lround(GameLayout::OrbHudTop)),
		static_cast<int>(std::lround(GameLayout::OrbHudRight)),
		static_cast<int>(std::lround(GameLayout::OrbHudBottom)));
	UiRenderer::DrawPanel(deviceContext, hud, false, UiTheme::Blue);
	UiRenderer::DrawText(
		deviceContext,
		CRect(hud.left + 10, hud.top + 12, hud.right - 10, hud.top + 42),
		_T("ORB QUEUE"),
		120,
		UiTheme::Gold);

	const CRect currentCard(hud.left + 10, hud.top + 50, hud.right - 10, hud.top + 175);
	UiRenderer::DrawPanel(deviceContext, currentCard, true, UiTheme::Gold);
	DrawOrbIcon(deviceContext, CRect(currentCard.left + 8, currentCard.top + 10, currentCard.left + 56, currentCard.top + 58), loadout.GetSelectedOrb());
	CString currentText;
	currentText.Format(
		_T("%s"),
		Text("label.current").GetString());
	UiRenderer::DrawText(deviceContext, CRect(currentCard.left + 60, currentCard.top + 12, currentCard.right - 8, currentCard.top + 40), currentText, 85, UiTheme::Gold, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	currentText.Format(
		_T("%s"),
		Utf8Text(loadout.GetSelectedOrb().displayName).GetString());
	UiRenderer::DrawText(deviceContext, CRect(currentCard.left + 8, currentCard.top + 62, currentCard.right - 8, currentCard.top + 92), currentText, 95, UiTheme::Text);
	UiRenderer::DrawText(deviceContext, CRect(currentCard.left + 8, currentCard.top + 92, currentCard.right - 8, currentCard.bottom - 6), AttackStyleText(loadout.GetSelectedOrb()), 80, UiTheme::MutedText);

	const CRect nextCard(hud.left + 10, hud.top + 185, hud.right - 10, hud.top + 310);
	UiRenderer::DrawPanel(deviceContext, nextCard, false, UiTheme::Blue);
	DrawOrbIcon(deviceContext, CRect(nextCard.left + 8, nextCard.top + 10, nextCard.left + 56, nextCard.top + 58), loadout.GetNextOrb());
	CString nextText;
	nextText.Format(
		_T("%s"),
		Text("label.next").GetString());
	UiRenderer::DrawText(deviceContext, CRect(nextCard.left + 60, nextCard.top + 12, nextCard.right - 8, nextCard.top + 40), nextText, 85, UiTheme::Blue, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	nextText.Format(
		_T("%s"),
		Utf8Text(loadout.GetNextOrb().displayName).GetString());
	UiRenderer::DrawText(deviceContext, CRect(nextCard.left + 8, nextCard.top + 62, nextCard.right - 8, nextCard.top + 92), nextText, 95, UiTheme::Text);
	UiRenderer::DrawText(deviceContext, CRect(nextCard.left + 8, nextCard.top + 92, nextCard.right - 8, nextCard.bottom - 6), AttackStyleText(loadout.GetNextOrb()), 80, UiTheme::MutedText);

	CString pileText;
	pileText.Format(
		_T("덱 %zu · 버림 %zu · 리필 %zu"),
		loadout.GetDrawPileCount(),
		loadout.GetDiscardPileCount(),
		loadout.GetReloadCount());
	UiRenderer::DrawText(deviceContext, CRect(hud.left + 8, hud.top + 318, hud.right - 8, hud.top + 342), pileText, 72, UiTheme::MutedText);
	UiRenderer::DrawText(deviceContext, CRect(hud.left + 8, hud.top + 342, hud.right - 8, hud.bottom - 6), RelicSummary(loadout), 72, UiTheme::Green, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
}

void CChildView::DrawCombatLog(CDC* deviceContext)
{
	UiRenderer::DrawText(
		deviceContext,
		CRect(800, 286, 970, 310),
		_combatLogVisible ? _T("[H] 전투 로그 닫기") : _T("[H] 전투 로그"),
		72,
		_combatLogVisible ? UiTheme::Gold : UiTheme::MutedText);
	if (!_combatLogVisible)
	{
		return;
	}

	const CRect panel(270, 438, 730, 680);
	UiRenderer::DrawPanel(deviceContext, panel, true, UiTheme::Gold);
	UiRenderer::DrawText(
		deviceContext,
		CRect(panel.left + 14, panel.top + 10, panel.right - 14, panel.top + 40),
		_T("전투 로그 · 최신 이벤트"),
		110,
		UiTheme::Gold);
	const auto& entries = _combatLog.GetEntries();
	const std::size_t first = entries.size() > 7 ? entries.size() - 7 : 0;
	int y = panel.top + 46;
	for (std::size_t index = first; index < entries.size(); ++index)
	{
		CString line;
		line.Format(
			_T("%02zu  %s"),
			entries[index].sequence,
			entries[index].text.c_str());
		UiRenderer::DrawText(
			deviceContext,
			CRect(panel.left + 16, y, panel.right - 16, y + 25),
			line,
			76,
			CombatToneColor(entries[index].tone),
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
		y += 26;
	}
}

void CChildView::DrawGameplayTooltip(CDC* deviceContext)
{
	if (!_pointerLogical.has_value())
	{
		return;
	}

	const Vector2 pointer = *_pointerLogical;
	const CRect currentCard(810, 370, 960, 495);
	const CRect nextCard(810, 505, 960, 630);
	const CRect relicArea(808, 638, 962, 678);
	CString title;
	CString detail;
	COLORREF accent = UiTheme::Blue;

	if (Contains(currentCard, pointer) || Contains(nextCard, pointer))
	{
		const bool current = Contains(currentCard, pointer);
		const OrbDefinition& orb = current
			? _game.GetLoadout().GetSelectedOrb()
			: _game.GetLoadout().GetNextOrb();
		title = (current ? _T("현재 오브 · ") : _T("다음 오브 · "))
			+ Utf8Text(orb.displayName);
		detail = Utf8Text(DescribeOrbEffect(orb));
		accent = OrbAccent(orb.id);
	}
	else if (Contains(relicArea, pointer))
	{
		title = _T("보유 유물 상세");
		for (const RelicDefinition& relic : GetRelicDefinitions())
		{
			const std::size_t stacks = _game.GetLoadout().GetRelicStackCount(relic.id);
			if (stacks == 0)
			{
				continue;
			}
			if (!detail.IsEmpty()) detail += _T("\n");
			CString relicLine;
			relicLine.Format(
				_T("%s x%zu · %s"),
				Utf8Text(relic.displayName).GetString(),
				stacks,
				Utf8Text(DescribeRelicEffect(relic)).GetString());
			detail += relicLine;
		}
		if (detail.IsEmpty()) detail = _T("현재 보유한 유물이 없습니다.");
		accent = UiTheme::Green;
	}
	else
	{
		const auto& enemies = _game.GetEnemies();
		const bool enemyGroup = enemies.size() > 1;
		for (std::size_t index = 0; index < enemies.size(); ++index)
		{
			const EnemyCombatant& enemy = enemies[index];
			if (!enemy.IsAlive()) continue;
			const Vector2 size = enemyGroup ? GameLayout::EnemyGroupSize : GameLayout::EnemySize;
			const float drawY = enemyGroup ? GameLayout::EnemyGroupY : GameLayout::EnemyInitialPosition.y;
			const CRect enemyBounds(
				static_cast<int>(std::lround(enemy.actor.GetX())),
				static_cast<int>(std::lround(drawY - 22.0f)),
				static_cast<int>(std::lround(enemy.actor.GetX() + size.x)),
				static_cast<int>(std::lround(drawY + size.y + 20.0f)));
			if (!Contains(enemyBounds, pointer)) continue;

			title.Format(
				_T("%s%s"),
				Utf8Text(enemy.definition.displayName).GetString(),
				index == _game.GetActiveEnemyIndex() ? _T(" · 현재 대상") : _T(""));
			detail.Format(
				_T("HP %d/%d · 피해 배율 ×%.2f\n거리 %d칸 · 공격 사거리 %d칸\n%s · 방어막 %d"),
				static_cast<int>(std::lround(enemy.actor.GetHp())),
				static_cast<int>(std::lround(enemy.definition.health)),
				enemy.definition.damageTakenMultiplier,
				enemy.distanceToPlayerCells,
				enemy.definition.attackRangeCells,
				EnemyActionText(_game.GetNextEnemyAction(index)).GetString(),
				static_cast<int>(std::lround(enemy.shield)));
			accent = enemy.IsPlayerInRange() ? UiTheme::Danger : UiTheme::Orange;
			break;
		}
	}

	if (title.IsEmpty())
	{
		return;
	}
	const CRect panel(260, 285, 740, 430);
	UiRenderer::DrawPanel(deviceContext, panel, true, accent);
	UiRenderer::DrawText(
		deviceContext,
		CRect(panel.left + 16, panel.top + 10, panel.right - 16, panel.top + 43),
		title,
		112,
		accent);
	UiRenderer::DrawText(
		deviceContext,
		CRect(panel.left + 20, panel.top + 48, panel.right - 20, panel.bottom - 12),
		detail,
		78,
		UiTheme::Text,
		DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
}

bool CChildView::StartStage(std::string_view stageId)
{
	const StageDefinition* stage = FindContentStage(_contentCatalog.stages, stageId);
	if (stage == nullptr || !_game.LoadStage(*stage, _runDifficulty))
	{
		return false;
	}

	_feedbackAnimations.clear();
	_attackAnimations.clear();
	_combatLog.Clear();
	_combatLog.Add(
		_T("전투 시작 · ") + std::wstring(Utf8Text(stage->displayName).GetString()),
		CombatLogTone::Neutral);
	_combatLogVisible = false;
	_terminalTransition.Reset();
	_orbTrail.clear();
	_orbTrailSampleSeconds = 0.0f;
	_gameplayVisualTimeSeconds = 0.0f;
	_resultSummary.reset();
	if (_run.GetClearedStageCount() > 0 && _runPlayerHealth > 0.0f)
	{
		_game.GetPlayer().SetHp((std::min)(_runPlayerHealth, _game.GetStage().rules.playerHealth));
	}
	_runPlayerHealth = _game.GetPlayer().GetHp();
	SetScreenMode(ScreenMode::Playing);
	SetFocus();
	return true;
}

bool CChildView::StartSelectedStage()
{
	if (_run.GetStatus() == RunStatus::StageReady)
	{
		if (IsRunShopStage(_run.GetCurrentStageId()))
		{
			_shopPurchased.fill(false);
			_runNotice = _T("상품은 각각 한 번만 구매할 수 있습니다");
			SetScreenMode(ScreenMode::Shop);
			SetFocus();
			return true;
		}
		return !_run.GetCurrentStageId().empty()
			&& StartStage(_run.GetCurrentStageId());
	}
	if (_run.GetStatus() != RunStatus::StageChoice
		|| _run.GetSelectedStageChoiceId().empty())
	{
		return false;
	}

	const std::string selectedStageId = _run.GetSelectedStageChoiceId();
	if (IsRunShopStage(selectedStageId))
	{
		if (!_run.ConfirmSelectedStage())
		{
			return false;
		}
		_shopPurchased.fill(false);
		_runNotice = _T("상품은 각각 한 번만 구매할 수 있습니다");
		SetScreenMode(ScreenMode::Shop);
		SetFocus();
		return true;
	}
	if (!StartStage(selectedStageId))
	{
		_runNotice = _T("스테이지를 시작할 수 없습니다");
		return false;
	}
	return _run.ConfirmSelectedStage();
}

void CChildView::BeginNewRun()
{
	std::vector<RunStageEntry> stageEntries;
	stageEntries.reserve(_contentCatalog.stages.size());
	for (const StageDefinition& stage : _contentCatalog.stages)
	{
		stageEntries.push_back({ stage.id, stage.isBoss });
	}
	RunStageLayers route = BuildBranchingStageLayers(stageEntries);

	_game.ResetProgression();
	if (!_run.StartBranching(std::move(route)))
	{
		std::vector<std::string> fallbackIds;
		for (const StageDefinition& stage : _contentCatalog.stages)
		{
			fallbackIds.push_back(stage.id);
		}
		_run.Start(std::move(fallbackIds));
	}
	_runDifficulty = _options.difficulty;
	_runPlayerHealth = 0.0f;
	_runNotice.Empty();
	_acquiredReward.reset();
	_rewardAcquisitionSeconds = 0.0f;
	_shopPurchased.fill(false);
	_resultSummary.reset();
	_feedbackAnimations.clear();
	_attackAnimations.clear();
	_terminalTransition.Reset();
	_orbTrail.clear();
	SetScreenMode(ScreenMode::StageSelection);
}

bool CChildView::SelectRunReward(std::size_t index)
{
	const std::vector<RunReward>& rewards = _run.GetRewardChoices();
	if (index >= rewards.size())
	{
		return false;
	}
	const RunReward candidate = rewards[index];

	switch (candidate.kind)
	{
	case RunRewardKind::Orb:
		if (!_game.AddOrb(candidate.id))
		{
			_runNotice.Format(
				_T("%s 획득 불가 · 오브 덱 보유 한도에 도달했습니다"),
				Utf8Text(candidate.displayName).GetString());
			return false;
		}
		break;
	case RunRewardKind::Relic:
		if (!_game.AcquireRelic(candidate.id))
		{
			_runNotice.Format(
				_T("%s 획득 불가 · 이미 유물 보유 한도에 도달했습니다"),
				Utf8Text(candidate.displayName).GetString());
			return false;
		}
		break;
	case RunRewardKind::Heal:
		_runPlayerHealth = (std::min)(
			_runPlayerHealth + candidate.magnitude,
			_game.GetStage().rules.playerHealth);
		break;
	}

	const std::optional<RunReward> selected = _run.SelectReward(index);
	if (!selected.has_value())
	{
		return false;
	}

	_acquiredReward = selected;
	_rewardAcquisitionSeconds = 0.0f;
	_runNotice.Format(
		_T("%s 획득 · %s"),
		Utf8Text(selected->displayName).GetString(),
		Utf8Text(DescribeRewardEffect(*selected)).GetString());
	SetScreenMode(ScreenMode::Reward);
	return true;
}

bool CChildView::PurchaseShopOffer(std::size_t index)
{
	const auto& offers = GetRunShopOffers();
	if (_screenMode != ScreenMode::Shop
		|| index >= offers.size()
		|| _shopPurchased[index])
	{
		_runNotice = index < _shopPurchased.size() && _shopPurchased[index]
			? _T("이미 구매한 상품입니다")
			: _T("구매할 수 없는 상품입니다");
		return false;
	}

	const RunShopOffer& offer = offers[index];
	if (_run.GetGold() < offer.price)
	{
		_runNotice.Format(_T("골드가 부족합니다 · 필요 %d G"), offer.price);
		return false;
	}

	bool applied = false;
	switch (offer.reward.kind)
	{
	case RunRewardKind::Orb:
		applied = _game.AddOrb(offer.reward.id);
		break;
	case RunRewardKind::Relic:
		applied = _game.AcquireRelic(offer.reward.id);
		break;
	case RunRewardKind::Heal:
	{
		const float maximumHealth = _game.GetStage().rules.playerHealth;
		if (_runPlayerHealth < maximumHealth)
		{
			_runPlayerHealth = (std::min)(
				_runPlayerHealth + offer.reward.magnitude,
				maximumHealth);
			applied = true;
		}
		break;
	}
	}

	if (!applied)
	{
		_runNotice = offer.reward.kind == RunRewardKind::Heal
			? _T("체력이 이미 최대입니다")
			: _T("보유 한도에 도달해 구매할 수 없습니다");
		return false;
	}
	if (!_run.SpendGold(offer.price))
	{
		_runNotice = _T("골드 결제에 실패했습니다");
		return false;
	}

	_shopPurchased[index] = true;
	_runNotice.Format(
		_T("%s 구매 완료 · 남은 골드 %d G"),
		Utf8Text(offer.reward.displayName).GetString(),
		_run.GetGold());
	return true;
}

bool CChildView::LeaveShop()
{
	if (_screenMode != ScreenMode::Shop || !_run.CompleteCurrentStage(false))
	{
		return false;
	}
	_runNotice = _T("상점을 나왔습니다 · 다음 경로를 선택하세요");
	SetScreenMode(ScreenMode::StageSelection);
	return true;
}

void CChildView::SaveOptions()
{
	_settingsSaveFailed = !_settingsStore.Save(_options);
}

void CChildView::RequestSelectiveReset(bool resetSettings)
{
	const ResetConfirmation requested = resetSettings
		? ResetConfirmation::Settings
		: ResetConfirmation::Records;
	if (_resetConfirmation != requested || _resetConfirmationSeconds <= 0.0f)
	{
		_resetConfirmation = requested;
		_resetConfirmationSeconds = 3.0f;
		_optionsNotice = resetSettings
			? _T("3초 안에 다시 누르면 설정만 초기화합니다")
			: _T("3초 안에 다시 누르면 전투 기록만 초기화합니다");
		return;
	}

	_resetConfirmation = ResetConfirmation::None;
	_resetConfirmationSeconds = 0.0f;
	std::string resetError;
	if (resetSettings)
	{
		if (!_settingsStore.Reset(&resetError))
		{
			_settingsSaveFailed = true;
			_optionsNotice = _T("설정 초기화 파일을 정리하지 못했습니다");
			return;
		}
		_options = GameOptions{};
		ApplyAudioOptions();
		ReloadLocalization();
		SaveOptions();
		_optionsNotice = _settingsSaveFailed
			? _T("설정은 초기화했지만 저장하지 못했습니다")
			: _T("설정만 기본값으로 초기화했습니다");
		return;
	}

	if (!_recordStore.Reset(&resetError))
	{
		_recordSaveFailed = true;
		_optionsNotice = _T("전투 기록 초기화 파일을 정리하지 못했습니다");
		return;
	}
	_records = GameRecordBook{};
	_recordSaveFailed = !_recordStore.Save(_records);
	_optionsNotice = _recordSaveFailed
		? _T("전투 기록은 초기화했지만 저장하지 못했습니다")
		: _T("전투 기록만 초기화했습니다");
}

void CChildView::ApplyAudioOptions()
{
	_audioPlayer.ApplyOptions(
		_options.soundEnabled,
		_options.effectsVolume,
		_options.musicVolume);
	UpdateScreenMusic();
}

void CChildView::UpdateScreenMusic()
{
	if (!_audioCatalog.IsUsable())
	{
		return;
	}
	if (_screenMode == ScreenMode::Playing)
	{
		_audioPlayer.StartMusic("battle_bgm");
	}
	else if (_screenMode == ScreenMode::Shop)
	{
		_audioPlayer.StartMusic("shop_bgm");
	}
	else
	{
		_audioPlayer.StartMusic("adventure_bgm");
	}
}

void CChildView::SetScreenMode(ScreenMode mode)
{
	if (_screenMode == mode)
	{
		return;
	}
	_screenMode = mode;
	_gamepadFocusIndex = 0;
	_resetConfirmation = ResetConfirmation::None;
	_resetConfirmationSeconds = 0.0f;
	_screenTransition.Start(0.28f);
	UpdateScreenMusic();
}

UiScreenKind CChildView::CurrentUiScreen() const noexcept
{
	switch (_screenMode)
	{
	case ScreenMode::StageSelection: return UiScreenKind::StageSelection;
	case ScreenMode::Loadout: return UiScreenKind::Loadout;
	case ScreenMode::Options: return UiScreenKind::Options;
	case ScreenMode::Statistics: return UiScreenKind::Statistics;
	case ScreenMode::Reward: return UiScreenKind::Reward;
	case ScreenMode::Shop: return UiScreenKind::Shop;
	case ScreenMode::Result: return UiScreenKind::Result;
	case ScreenMode::Playing: return UiScreenKind::StageSelection;
	}
	return UiScreenKind::StageSelection;
}

std::size_t CChildView::VisibleStageCount() const noexcept
{
	if (_run.GetStatus() == RunStatus::StageChoice)
	{
		return _run.GetAvailableStageIds().size();
	}
	return _run.GetStatus() == RunStatus::StageReady ? std::size_t{ 1 } : std::size_t{ 0 };
}

void CChildView::PollGamepad()
{
	XINPUT_STATE state{};
	if (::XInputGetState(0, &state) != ERROR_SUCCESS)
	{
		_gamepadConnected = false;
		_previousGamepadButtons = 0;
		_gamepadTriggerDown = false;
		_gamepadStickLatched = false;
		return;
	}

	_gamepadConnected = true;
	const DWORD buttons = state.Gamepad.wButtons;
	const DWORD pressed = buttons & ~_previousGamepadButtons;
	_previousGamepadButtons = buttons;
	const bool triggerDown = state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
	const bool triggerPressed = triggerDown && !_gamepadTriggerDown;
	_gamepadTriggerDown = triggerDown;

	const float stickX = static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f;
	const float stickY = -static_cast<float>(state.Gamepad.sThumbLY) / 32767.0f;
	const Vector2 rawStick{ stickX, stickY };
	const Vector2 stick = ApplyGamepadStickTuning(
		rawStick,
		_options.gamepadDeadzonePercent,
		_options.gamepadSensitivityPercent);
	const float stickLength = stick.Length();

	if (_screenMode == ScreenMode::Playing)
	{
		if ((pressed & XINPUT_GAMEPAD_START) != 0U)
		{
			_game.TogglePause();
		}
		if ((pressed & XINPUT_GAMEPAD_B) != 0U && _game.GetBall().GetClick())
		{
			_game.ResetBallToAiming();
			return;
		}
		if (_game.GetState() == GameState::Aiming && stickLength > 0.0f)
		{
			_gamepadAimDirection = stick.Normalized();
			const Vector2 ballPosition = _game.GetBall().GetPosition();
			if (!_game.GetBall().GetClick())
			{
				_game.BeginAim(ballPosition);
			}
			const float dragDistance = 100.0f + stickLength * 100.0f;
			const Vector2 dragPosition = ballPosition - _gamepadAimDirection * dragDistance;
			_game.UpdateAim(dragPosition);
			if (ShouldFireGamepadShot(
				_options.gamepadFireBinding,
				(pressed & XINPUT_GAMEPAD_A) != 0U,
				triggerPressed))
			{
				_game.ReleaseShot(dragPosition);
			}
		}
		return;
	}

	const bool previous = (buttons & (XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP)) != 0U
		|| stickX <= -0.55f || stickY <= -0.55f;
	const bool next = (buttons & (XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN)) != 0U
		|| stickX >= 0.55f || stickY >= 0.55f;
	const bool navigationHeld = previous || next;
	if (navigationHeld && !_gamepadStickLatched)
	{
		_gamepadFocusIndex = MoveGamepadFocus(
			_gamepadFocusIndex,
			GetGamepadFocusCount(CurrentUiScreen(), VisibleStageCount()),
			previous ? -1 : 1);
	}
	_gamepadStickLatched = navigationHeld;

	if ((pressed & XINPUT_GAMEPAD_A) != 0U)
	{
		ExecuteUiAction(ResolveGamepadFocusedAction(
			CurrentUiScreen(),
			_gamepadFocusIndex,
			VisibleStageCount()));
	}
	else if ((pressed & XINPUT_GAMEPAD_B) != 0U)
	{
		ExecuteUiAction(ResolveGamepadBackAction(CurrentUiScreen()));
	}
}

void CChildView::RecordResult(bool cleared)
{
	if (!_resultSummary.has_value())
	{
		return;
	}

	bool changed = _records.ApplyResult(
		_resultSummary->stageId,
		_options.difficulty,
		_resultSummary->totalScore,
		_resultSummary->bestCombo,
		cleared);
	changed = _records.ApplyPerformanceResult(
		_resultSummary->stageId,
		_options.difficulty,
		_resultSummary->orbId,
		_resultSummary->totalScore,
		_resultSummary->bestCombo,
		cleared) || changed;
	if (changed)
	{
		_recordSaveFailed = !_recordStore.Save(_records);
	}
}

void CChildView::PlayEventSound(GameEventType eventType, PegType pegType)
{
	std::string_view cue = "attack";
	if (eventType == GameEventType::PegHit) cue = "peg_hit";
	else if (eventType == GameEventType::BombTriggered || pegType == PegType::Critical) cue = "bomb";
	else if (eventType == GameEventType::RefreshTriggered
		|| eventType == GameEventType::RefreshGuaranteed
		|| eventType == GameEventType::RefreshRelocated
		|| pegType == PegType::Refresh) cue = "refresh";
	else if (eventType == GameEventType::PlayerDamaged) cue = "damage";
	else if (eventType == GameEventType::Victory) cue = "victory";
	else if (eventType == GameEventType::Defeat) cue = "defeat";
	_audioPlayer.PlayEffect(cue);
}


void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPoint logicalPoint;
	if (!TryMapClientPoint(point, logicalPoint, false))
	{
		CWnd::OnLButtonDown(nFlags, point);
		return;
	}
	if (_screenMode != ScreenMode::Playing)
	{
		if (HandleMenuClick(logicalPoint))
		{
			SetFocus();
			Invalidate(FALSE);
		}
		CWnd::OnLButtonDown(nFlags, point);
		return;
	}

	if (_game.BeginAim({ static_cast<float>(logicalPoint.x), static_cast<float>(logicalPoint.y) }))
	{
		SetFocus();
		SetCapture();
	}
	CWnd::OnLButtonDown(nFlags, point);
}

bool CChildView::TryMapClientPoint(
	CPoint clientPoint,
	CPoint& logicalPoint,
	bool clampToViewport) const
{
	CRect clientBounds;
	GetClientRect(&clientBounds);
	const UiViewport viewport = CreateUiViewport(clientBounds.Width(), clientBounds.Height());
	Vector2 logical;
	if (!viewport.TryClientToLogical(
		{ static_cast<float>(clientPoint.x), static_cast<float>(clientPoint.y) },
		logical,
		clampToViewport))
	{
		return false;
	}
	logicalPoint = CPoint(
		static_cast<int>(std::lround(logical.x)),
		static_cast<int>(std::lround(logical.y)));
	return true;
}

bool CChildView::HandleMenuClick(CPoint point)
{
	UiScreenKind screen = UiScreenKind::StageSelection;
	switch (_screenMode)
	{
	case ScreenMode::StageSelection: screen = UiScreenKind::StageSelection; break;
	case ScreenMode::Loadout: screen = UiScreenKind::Loadout; break;
	case ScreenMode::Options: screen = UiScreenKind::Options; break;
	case ScreenMode::Statistics: screen = UiScreenKind::Statistics; break;
	case ScreenMode::Reward: screen = UiScreenKind::Reward; break;
	case ScreenMode::Shop: screen = UiScreenKind::Shop; break;
	case ScreenMode::Result: screen = UiScreenKind::Result; break;
	case ScreenMode::Playing: return false;
	}

	const std::size_t stageCount = _run.GetStatus() == RunStatus::StageChoice
		? _run.GetAvailableStageIds().size()
		: (_run.GetStatus() == RunStatus::StageReady ? std::size_t{ 1 } : std::size_t{ 0 });
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
	if (action.command != UiCommand::None)
	{
		_audioPlayer.PlayEffect("ui_confirm");
	}
	switch (action.command)
	{
	case UiCommand::SelectStage:
		if (_run.GetStatus() == RunStatus::StageChoice
			&& _run.SelectNextStage(action.index))
		{
			const StageDefinition* selected = FindContentStage(
				_contentCatalog.stages,
				_run.GetSelectedStageChoiceId());
			if (selected != nullptr)
			{
				_runNotice.Format(
					_T("다음 경로: %s"),
					Utf8Text(selected->displayName).GetString());
			}
			else if (IsRunShopStage(_run.GetSelectedStageChoiceId()))
			{
				_runNotice = _T("다음 경로: Goblin Market · 시작 전 변경 가능");
			}
		}
		break;
	case UiCommand::StartSelectedStage:
		StartSelectedStage();
		break;
	case UiCommand::OpenLoadout:
		_loadoutNotice.Empty();
		SetScreenMode(ScreenMode::Loadout);
		break;
	case UiCommand::OpenOptions:
		SetScreenMode(ScreenMode::Options);
		break;
	case UiCommand::OpenStatistics:
		SetScreenMode(ScreenMode::Statistics);
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
		SetScreenMode(ScreenMode::StageSelection);
		break;
	case UiCommand::ToggleDifficulty:
		_options.CycleDifficulty();
		SaveOptions();
		break;
	case UiCommand::ToggleSound:
		_options.ToggleSound();
		ApplyAudioOptions();
		SaveOptions();
		break;
	case UiCommand::CycleEffectsVolume:
		_options.CycleEffectsVolume();
		ApplyAudioOptions();
		SaveOptions();
		break;
	case UiCommand::CycleMusicVolume:
		_options.CycleMusicVolume();
		ApplyAudioOptions();
		SaveOptions();
		break;
	case UiCommand::TogglePegColorMode:
		_options.TogglePegColorMode();
		SaveOptions();
		break;
	case UiCommand::ToggleLanguage:
		_options.ToggleLanguage();
		ReloadLocalization();
		SaveOptions();
		break;
	case UiCommand::CycleGamepadDeadzone:
		_options.CycleGamepadDeadzone();
		SaveOptions();
		break;
	case UiCommand::CycleGamepadSensitivity:
		_options.CycleGamepadSensitivity();
		SaveOptions();
		break;
	case UiCommand::ToggleGamepadFireBinding:
		_options.ToggleGamepadFireBinding();
		SaveOptions();
		break;
	case UiCommand::ResetSettingsData:
		RequestSelectiveReset(true);
		break;
	case UiCommand::ResetRecordData:
		RequestSelectiveReset(false);
		break;
	case UiCommand::CycleStatisticsDifficulty:
		_statisticsDifficulty = NextStatisticsDifficultyFilter(_statisticsDifficulty);
		break;
	case UiCommand::CycleStatisticsSort:
		_statisticsSort = NextStatisticsSortMode(_statisticsSort);
		break;
	case UiCommand::SelectReward:
		SelectRunReward(action.index);
		break;
	case UiCommand::BuyShopOffer:
		PurchaseShopOffer(action.index);
		break;
	case UiCommand::LeaveShop:
		LeaveShop();
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

	CPoint logicalPoint;
	if (_game.GetBall().GetClick()
		&& TryMapClientPoint(point, logicalPoint, true))
	{
		_game.ReleaseShot({
			static_cast<float>(logicalPoint.x),
			static_cast<float>(logicalPoint.y) });
	}
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
	CPoint pointerPoint;
	if (TryMapClientPoint(point, pointerPoint, false))
	{
		_pointerLogical = Vector2{
			static_cast<float>(pointerPoint.x),
			static_cast<float>(pointerPoint.y) };
	}
	else
	{
		_pointerLogical.reset();
	}
	if (_screenMode != ScreenMode::Playing)
	{
		CWnd::OnMouseMove(nFlags, point);
		return;
	}

	//드래그 처리
	if (_game.GetBall().GetClick())
	{
		CPoint logicalPoint;
		if (TryMapClientPoint(point, logicalPoint, true))
		{
			_game.UpdateAim({
				static_cast<float>(logicalPoint.x),
				static_cast<float>(logicalPoint.y) });
			Invalidate(FALSE);
		}
	}
	else
	{
		Invalidate(FALSE);
	}

	CWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_F9)
	{
		SetDemoMode(!_demoRun.IsEnabled());
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}
	if (_screenMode == ScreenMode::StageSelection)
	{
		if (nChar == VK_RETURN
			&& (_run.GetStatus() == RunStatus::StageReady
				|| (_run.GetStatus() == RunStatus::StageChoice
					&& !_run.GetSelectedStageChoiceId().empty())))
		{
			StartSelectedStage();
		}
		else if (nChar >= '1' && nChar <= '2')
		{
			const std::size_t requested = static_cast<std::size_t>(nChar - '1');
			if (_run.GetStatus() == RunStatus::StageChoice)
			{
				ExecuteUiAction({ UiCommand::SelectStage, requested });
			}
			else if (_run.GetStatus() == RunStatus::StageReady && requested == 0)
			{
				StartSelectedStage();
			}
		}
		else if (nChar == 'O')
		{
			SetScreenMode(ScreenMode::Options);
		}
		else if (nChar == 'L')
		{
			_loadoutNotice.Empty();
			SetScreenMode(ScreenMode::Loadout);
		}
		else if (nChar == 'T')
		{
			SetScreenMode(ScreenMode::Statistics);
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
			SetScreenMode(ScreenMode::StageSelection);
		}
		Invalidate();
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	if (_screenMode == ScreenMode::Statistics)
	{
		if (nChar == 'D')
		{
			_statisticsDifficulty = NextStatisticsDifficultyFilter(_statisticsDifficulty);
		}
		else if (nChar == 'S')
		{
			_statisticsSort = NextStatisticsSortMode(_statisticsSort);
		}
		else if (nChar == 'B' || nChar == VK_ESCAPE)
		{
			SetScreenMode(ScreenMode::StageSelection);
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
			ApplyAudioOptions();
			SaveOptions();
		}
		else if (nChar == 'E')
		{
			_options.CycleEffectsVolume();
			ApplyAudioOptions();
			SaveOptions();
		}
		else if (nChar == 'V')
		{
			_options.CycleMusicVolume();
			ApplyAudioOptions();
			SaveOptions();
		}
		else if (nChar == 'C')
		{
			_options.TogglePegColorMode();
			SaveOptions();
		}
		else if (nChar == 'L')
		{
			_options.ToggleLanguage();
			ReloadLocalization();
			SaveOptions();
		}
		else if (nChar == 'Z')
		{
			_options.CycleGamepadDeadzone();
			SaveOptions();
		}
		else if (nChar == 'G')
		{
			_options.CycleGamepadSensitivity();
			SaveOptions();
		}
		else if (nChar == 'F')
		{
			_options.ToggleGamepadFireBinding();
			SaveOptions();
		}
		else if (nChar == 'X')
		{
			RequestSelectiveReset(true);
		}
		else if (nChar == 'R')
		{
			RequestSelectiveReset(false);
		}
		else if (nChar == 'B' || nChar == VK_ESCAPE)
		{
			SetScreenMode(ScreenMode::StageSelection);
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

	if (_screenMode == ScreenMode::Shop)
	{
		if (nChar >= '1' && nChar <= '3')
		{
			PurchaseShopOffer(static_cast<std::size_t>(nChar - '1'));
		}
		else if (nChar == VK_RETURN || nChar == VK_ESCAPE || nChar == 'B')
		{
			LeaveShop();
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
		ApplyAudioOptions();
		SaveOptions();
		Invalidate();
	}
	if (nChar == 'H')
	{
		_combatLogVisible = !_combatLogVisible;
		Invalidate(FALSE);
	}
	if (nChar == 'S' && _game.GetState() == GameState::Aiming)
	{
		ReleaseMouseInput(true);
		_game.ResetGame();
		_feedbackAnimations.clear();
		_resultSummary.reset();
		SetScreenMode(ScreenMode::StageSelection);
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

void CChildView::OnToggleGameplayInfo()
{
	_options.ToggleGameplayInfo();
	SaveOptions();
	Invalidate(FALSE);
}

void CChildView::OnUpdateGameplayInfo(CCmdUI* commandUi)
{
	if (commandUi != nullptr)
	{
		commandUi->SetCheck(_options.showGameplayInfo ? 1 : 0);
	}
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
