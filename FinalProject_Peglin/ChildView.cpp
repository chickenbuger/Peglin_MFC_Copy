
// ChildView.cpp: CChildView 클래스의 구현
//

#include "pch.h"
#include "framework.h"
#include "FinalProject_Peglin.h"
#include "ChildView.h"
#include "GameLayout.h"
#include "RewardPresentation.h"
#include "SoftPegSound.h"
#include <algorithm>
#include <cmath>
#include <mmsystem.h>
#include <utility>

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "Winmm.lib")

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
	ReloadLocalization();
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
	_screenMode = _run.GetStatus() == RunStatus::RewardSelection
		? ScreenMode::Reward
		: ScreenMode::Result;
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
	_screenMode = ScreenMode::Result;
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
	if (_screenMode == ScreenMode::Shop)
	{
		DrawShopScreen(&memDc);
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
		const EnemyCombatant& activeEnemy = enemyRoster[_game.GetActiveEnemyIndex()];
		CString enemyAction;
		enemyAction.Format(
			_T("거리 %d칸 · 사거리 %d칸 · "),
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
		memDc.TextOut(
			680,
			static_cast<int>(std::lround(GameLayout::EnemyHealthTextY + 20.0f)),
			enemyAction);
	}
	memDc.RestoreDC(statusTextState);

	if (_options.showGameplayInfo)
	{
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
			DifficultyTextForUi(_game.GetDifficulty()).GetString(),
			_options.soundEnabled ? _T("소리") : _T("음소거"),
			_options.pegColorMode == PegColorMode::HighContrast ? _T("고대비") : _T("표준"));
		memDc.TextOut(
			static_cast<int>(std::lround(GameLayout::OptionsText.x)),
			static_cast<int>(std::lround(GameLayout::OptionsText.y)),
			optionsText);
		memDc.RestoreDC(textState);
	}
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

	return 0;
}

void CChildView::OnDestroy()
{
	ReleaseMouseInput(true);
	::PlaySoundW(nullptr, nullptr, 0);
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
				_screenMode = ScreenMode::StageSelection;
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
			break;
		case GameEventType::EnemyAdvanced:
			animation.text.Format(_T("1칸 전진 · 거리 %d칸"), event.affectedPegs);
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
		const float progress = animation.ageSeconds / animation.lifetimeSeconds;
		deviceContext->SetTextColor(animation.color);
		deviceContext->TextOut(
			static_cast<int>(std::lround(animation.position.x - 30.0f)),
			static_cast<int>(std::lround(animation.position.y - progress * 45.0f)),
			animation.text);
	}

	deviceContext->RestoreDC(savedDc);
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
	UiRenderer::DrawText(deviceContext, CRect(230, 20, 970, 78), Text("screen.stage_selection"), 265, UiTheme::Gold);

	const CRect statusPanel(24, 105, 225, 700);
	UiRenderer::DrawPanel(deviceContext, statusPanel);
	UiRenderer::DrawText(deviceContext, CRect(40, 122, 209, 164), _T("ADVENTURE"), 150, UiTheme::Gold);
	CString runProgress;
	runProgress.Format(
		_T("경로 %zu / %zu\n체력 %d\n골드 %d"),
		(std::min)(_run.GetClearedStageCount() + 1, _run.GetStageCount()),
		_run.GetStageCount(),
		static_cast<int>(std::lround(_runPlayerHealth > 0.0f ? _runPlayerHealth : _game.GetPlayer().GetHp())),
		_run.GetGold());
	UiRenderer::DrawText(deviceContext, CRect(42, 178, 207, 255), runProgress, 105, UiTheme::Green, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawText(deviceContext, CRect(42, 278, 207, 308), _T("현재 오브"), 100, UiTheme::MutedText);
	UiRenderer::DrawText(
		deviceContext,
		CRect(42, 310, 207, 350),
		Utf8Text(_game.GetLoadout().GetSelectedOrb().displayName),
		130,
		UiTheme::Text);
	UiRenderer::DrawText(deviceContext, CRect(42, 372, 207, 402), _T("보유 유물"), 100, UiTheme::MutedText);
	UiRenderer::DrawText(
		deviceContext,
		CRect(43, 405, 206, 555),
		RelicSummary(_game.GetLoadout()),
		95,
		UiTheme::Green,
		DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawKeyHint(deviceContext, CRect(43, 575, 206, 625), Text("hint.loadout"));
	UiRenderer::DrawKeyHint(deviceContext, CRect(43, 638, 206, 688), Text("hint.options"));

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

	if (_acquiredReward.has_value())
	{
		const RunReward& acquired = *_acquiredReward;
		const COLORREF color = acquired.kind == RunRewardKind::Orb
			? UiTheme::Blue
			: (acquired.kind == RunRewardKind::Relic ? UiTheme::Gold : UiTheme::Green);
		UiRenderer::DrawText(deviceContext, CRect(180, 35, 800, 110), _T("보상 획득 완료"), 300, UiTheme::Gold);
		UiRenderer::DrawText(deviceContext, CRect(160, 125, 820, 170), _T("효과 적용을 확인한 뒤 다음 경로로 이동합니다"), 125, UiTheme::MutedText);
		const CRect acquiredCard(270, 190, 730, 540);
		UiRenderer::DrawPanel(deviceContext, acquiredCard, true, color);
		DrawRewardIcon(acquired, CRect(450, 220, 550, 320));
		UiRenderer::DrawText(deviceContext, CRect(300, 325, 700, 375), Utf8Text(acquired.displayName), 180, color);
		UiRenderer::DrawText(
			deviceContext,
			CRect(310, 385, 690, 510),
			Utf8Text(DescribeRewardEffect(acquired)),
			110,
			UiTheme::Text,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
		UiRenderer::DrawText(deviceContext, CRect(280, 575, 720, 615), _T("획득 효과 적용 완료"), 115, UiTheme::Green);
		return;
	}

	UiRenderer::DrawText(deviceContext, CRect(180, 35, 800, 110), Text("screen.reward"), 300, UiTheme::Gold);
	UiRenderer::DrawText(deviceContext, CRect(120, 120, 880, 175), _T("효과 수치와 보유 한도를 확인한 뒤 보상 하나를 선택하세요"), 125, UiTheme::MutedText);

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
		UiRenderer::DrawText(deviceContext, CRect(left + 15, 253, left + 255, 285), title, 120, color);
		DrawRewardIcon(reward, CRect(left + 99, 285, left + 171, 357));
		UiRenderer::DrawText(deviceContext, CRect(left + 12, 358, left + 258, 392), Utf8Text(reward.displayName), 125, UiTheme::Text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 12, 395, left + 258, 478),
			Utf8Text(DescribeRewardEffect(reward)),
			82,
			UiTheme::MutedText,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	}
	UiRenderer::DrawText(deviceContext, CRect(120, 525, 880, 595), _runNotice, 105, UiTheme::Orange, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawKeyHint(deviceContext, CRect(260, 640, 720, 690), _T("보상 카드 클릭 · 1/2/3 선택"));
}

void CChildView::DrawShopScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	UiRenderer::DrawText(deviceContext, CRect(190, 35, 810, 105), _T("GOBLIN MARKET"), 285, UiTheme::Gold);
	CString wallet;
	wallet.Format(_T("보유 골드  %d G"), _run.GetGold());
	UiRenderer::DrawText(deviceContext, CRect(300, 110, 930, 150), wallet, 145, UiTheme::Gold);

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
		const int left = 300 + static_cast<int>(index) * 215;
		const CRect card(left, 190, left + 200, 520);
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
		UiRenderer::DrawText(deviceContext, CRect(left + 8, 200, left + 192, 232), header, 95, categoryColor);

		if (offer.reward.kind == RunRewardKind::Orb)
		{
			const OrbDefinition* orb = FindOrbDefinition(offer.reward.id);
			if (orb != nullptr)
			{
				DrawOrbIcon(deviceContext, CRect(left + 64, 238, left + 136, 310), *orb);
			}
		}
		else if (offer.reward.kind == RunRewardKind::Relic)
		{
			const RelicDefinition* relic = FindRelicDefinition(offer.reward.id);
			if (relic != nullptr)
			{
				DrawRelicIcon(deviceContext, CRect(left + 64, 238, left + 136, 310), *relic);
			}
		}
		else
		{
			DrawHealIcon(deviceContext, CRect(left + 64, 238, left + 136, 310));
		}

		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 8, 315, left + 192, 350),
			Utf8Text(offer.reward.displayName),
			105,
			UiTheme::Text);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 10, 355, left + 190, 445),
			Utf8Text(DescribeRewardEffect(offer.reward)),
			70,
			UiTheme::MutedText,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 10, 465, left + 190, 505),
			purchased ? _T("구매 완료") : (affordable ? _T("구매 가능") : _T("골드 부족")),
			95,
			purchased ? UiTheme::Green : (affordable ? UiTheme::Gold : UiTheme::Danger));
	}

	UiRenderer::DrawText(deviceContext, CRect(300, 545, 930, 610), _runNotice, 95, UiTheme::Orange, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
	UiRenderer::DrawKeyHint(deviceContext, CRect(340, 640, 660, 690), _T("상점 나가기 · ENTER / ESC"));
}

void CChildView::DrawLoadoutScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	UiRenderer::DrawText(deviceContext, CRect(160, 35, 820, 110), Text("screen.loadout"), 285, UiTheme::Gold);
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
		DrawOrbIcon(deviceContext, CRect(left + 14, 198, left + 68, 252), orb);
		CString title;
		title.Format(_T("[%zu] %s · x%zu"), index + 1, Utf8Text(orb.displayName).GetString(), ownedCount);
		UiRenderer::DrawText(deviceContext, CRect(left + 72, 198, left + 260, 252), title, 115, selected ? UiTheme::Gold : UiTheme::Text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 12, 255, left + 258, 327),
			Utf8Text(DescribeOrbEffect(orb)),
			78,
			UiTheme::MutedText,
			DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
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
		title.Format(_T("[%zu] %s · %zu/%zu"), index + 4, Utf8Text(relic.displayName).GetString(), stacks, relic.maxStacks);
		DrawRelicIcon(deviceContext, CRect(left + 14, 402, left + 68, 456), relic);
		UiRenderer::DrawText(deviceContext, CRect(left + 72, 400, left + 260, 458), title, 108, atLimit ? UiTheme::Green : UiTheme::Text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		UiRenderer::DrawText(
			deviceContext,
			CRect(left + 12, 462, left + 258, 548),
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
	UiRenderer::DrawText(deviceContext, CRect(300, 55, 680, 120), Text("screen.options"), 300, UiTheme::Gold);
	CString difficulty;
	difficulty.Format(
		_T("[D] %s    %s"),
		Text("option.difficulty").GetString(),
		DifficultyTextForUi(_options.difficulty).GetString());
	UiRenderer::DrawPanel(deviceContext, CRect(285, 205, 695, 285));
	UiRenderer::DrawText(deviceContext, CRect(300, 215, 680, 275), difficulty, 165);
	CString sound;
	sound.Format(
		_T("[M] %s    %s"),
		Text("option.sound").GetString(),
		Text(_options.soundEnabled ? "value.on" : "value.off").GetString());
	UiRenderer::DrawPanel(deviceContext, CRect(285, 315, 695, 395));
	UiRenderer::DrawText(deviceContext, CRect(300, 325, 680, 385), sound, 165);
	CString colorMode;
	colorMode.Format(
		_T("[C] %s    %s"),
		Text("option.peg_color").GetString(),
		PegColorModeTextForUi(_options.pegColorMode).GetString());
	UiRenderer::DrawPanel(deviceContext, CRect(285, 425, 695, 505));
	UiRenderer::DrawText(deviceContext, CRect(300, 435, 680, 495), colorMode, 155);
	CString language;
	language.Format(
		_T("[L] %s    %s"),
		Text("option.language").GetString(),
		Text(_options.language == UiLanguage::Korean
			? "language.korean"
			: "language.english").GetString());
	UiRenderer::DrawPanel(deviceContext, CRect(285, 520, 695, 600));
	UiRenderer::DrawText(deviceContext, CRect(300, 530, 680, 590), language, 155);
	UiRenderer::DrawText(
		deviceContext,
		CRect(275, 605, 705, 635),
		_settingsSaveFailed ? Text("notice.settings_save_failed") : Text("notice.auto_save"),
		95,
		_settingsSaveFailed ? UiTheme::Danger : UiTheme::Green);
	UiRenderer::DrawKeyHint(deviceContext, CRect(300, 640, 680, 690), Text("hint.back"));
}

void CChildView::DrawResultScreen(CDC* deviceContext)
{
	DrawMenuBackdrop(deviceContext);
	const bool victory = _resultSummary.has_value()
		&& _resultSummary->result == GameUpdateResult::Victory;
	UiRenderer::DrawPanel(deviceContext, CRect(250, 135, 730, 650), true, victory ? UiTheme::Green : UiTheme::Danger);
	UiRenderer::DrawText(
		deviceContext,
		CRect(250, 55, 730, 125),
		Text(victory ? "screen.run_complete" : "screen.run_failed"),
		300,
		victory ? UiTheme::Green : UiTheme::Danger);
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
	UiRenderer::DrawKeyHint(deviceContext, CRect(260, 640, 480, 690), Text("hint.retry"));
	UiRenderer::DrawKeyHint(deviceContext, CRect(500, 640, 720, 690), Text("hint.new_run"));
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

bool CChildView::StartStage(std::string_view stageId)
{
	const StageDefinition* stage = FindContentStage(_contentCatalog.stages, stageId);
	if (stage == nullptr || !_game.LoadStage(*stage, _runDifficulty))
	{
		return false;
	}

	_feedbackAnimations.clear();
	_attackAnimations.clear();
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
	_screenMode = ScreenMode::Playing;
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
			_screenMode = ScreenMode::Shop;
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
		_screenMode = ScreenMode::Shop;
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
	_screenMode = ScreenMode::StageSelection;
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
	_screenMode = ScreenMode::Reward;
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

	if (eventType == GameEventType::PegHit)
	{
		static const std::vector<std::uint8_t> pegHitWave = CreateSoftPegHitWave();
		::PlaySoundW(
			reinterpret_cast<LPCWSTR>(pegHitWave.data()),
			nullptr,
			SND_MEMORY | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
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
	case UiCommand::ToggleLanguage:
		_options.ToggleLanguage();
		ReloadLocalization();
		SaveOptions();
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
		else if (nChar == 'L')
		{
			_options.ToggleLanguage();
			ReloadLocalization();
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
