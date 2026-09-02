
// ChildView.h: CChildView 클래스의 인터페이스
//


#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <string_view>
#include <vector>

#include "Background.h"
#include "AudioCatalog.h"
#include "AudioPlayer.h"
#include "CombatLog.h"
#include "ContentCatalog.h"
#include "ContentReload.h"
#include "DemoRun.h"
#include "GameRecordStore.h"
#include "GameSettingsStore.h"
#include "GamepadFeedback.h"
#include "GamepadNavigation.h"
#include "GameplayCatalog.h"
#include "GameStatistics.h"
#include "GameWorld.h"
#include "Localization.h"
#include "PerformanceMonitor.h"
#include "RunProgression.h"
#include "RunCheckpointStore.h"
#include "TerminalTransitionGate.h"
#include "PeglinUiAnimation.h"
#include "UiNavigation.h"
#include "UiRenderer.h"
#include "UiViewport.h"

// CChildView 창

class CChildView : public CWnd
{
// 생성입니다.
public:
	CChildView();
	virtual ~CChildView();

// 특성입니다.
public:
	void gameclear();
	void gameover();
	void restart();
	GameState GetGameState() const noexcept { return _game.GetState(); }

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
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCaptureChanged(CWnd* pWnd);
	afx_msg void On32771();
	afx_msg void OnToggleGameplayInfo();
	afx_msg void OnUpdateGameplayInfo(CCmdUI* commandUi);

private:
	enum class ScreenMode
	{
		StageSelection,
		Loadout,
		Options,
		Statistics,
		Playing,
		Reward,
		Shop,
		Result
	};

	enum class ResetConfirmation
	{
		None,
		Settings,
		Records
	};

	struct FeedbackAnimation
	{
		CString text;
		Vector2 position;
		COLORREF color = RGB(255, 255, 255);
		float ageSeconds = 0.0f;
		float lifetimeSeconds = 0.9f;
		bool toast = false;
	};

	struct OrbTrailPoint
	{
		Vector2 position;
		float ageSeconds = 0.0f;
	};

	struct AttackAnimation
	{
		AttackDelivery delivery = AttackDelivery::Projectile;
		AttackTarget target = AttackTarget::Single;
		Vector2 start;
		Vector2 end;
		COLORREF color = RGB(255, 194, 62);
		float ageSeconds = 0.0f;
		float lifetimeSeconds = 0.65f;
	};

	void ReleaseMouseInput(bool cancelDrag);
	void BeginAimFromPointer(CPoint point);
	void EndAimFromPointer(CPoint point);
	void UpdateGameStep(float deltaSeconds);
	void ConsumeGameEvents();
	void UpdateFeedbackAnimations(float deltaSeconds);
	void UpdateAttackAnimations(float deltaSeconds);
	void UpdateOrbVisuals(float deltaSeconds);
	void FinishPendingTerminalTransition();
	void DrawFeedbackAnimations(CDC* deviceContext);
	void DrawUiAnimations(CDC* deviceContext, const CRect& clientBounds);
	void DrawGamepadFocus(CDC* deviceContext);
	void DrawAttackAnimations(CDC* deviceContext);
	void DrawOrbTrail(CDC* deviceContext);
	void DrawAimPreview(CDC* deviceContext);
	void DrawCombatLog(CDC* deviceContext);
	void DrawGameplayTooltip(CDC* deviceContext);
	void DrawDemoBadge(CDC* deviceContext);
	void DrawPerformanceHud(CDC* deviceContext);
	void DrawEnemyHealthBar(
		CDC* deviceContext,
		const EnemyCombatant& combatant,
		Vector2 drawSize,
		float drawY,
		bool enemyGroup,
		bool activeTarget);
	void DrawStageSelection(CDC* deviceContext);
	void DrawContentPreview(CDC* deviceContext);
	void DrawLoadoutScreen(CDC* deviceContext);
	void DrawOptions(CDC* deviceContext);
	void DrawStatisticsScreen(CDC* deviceContext);
	void DrawRewardScreen(CDC* deviceContext);
	void DrawShopScreen(CDC* deviceContext);
	void DrawResultScreen(CDC* deviceContext);
	void DrawMenuBackdrop(CDC* deviceContext);
	void DrawPlayingLoadout(CDC* deviceContext);
	void DrawOrbIcon(CDC* deviceContext, const CRect& bounds, const OrbDefinition& orb);
	void DrawRelicIcon(CDC* deviceContext, const CRect& bounds, const RelicDefinition& relic);
	bool HandleMenuClick(CPoint point);
	bool HandleContentPreviewClick(CPoint point);
	bool TryMapClientPoint(CPoint clientPoint, CPoint& logicalPoint, bool clampToViewport) const;
	void ExecuteUiAction(const UiAction& action);
	CBitmap* GetEnemySprite(EnemyVisualKind visual) noexcept;
	CBitmap* GetOrbIcon(std::string_view imageKey) noexcept;
	CBitmap* GetRelicIcon(std::string_view imageKey) noexcept;
	CBitmap* GetStagePreview(std::string_view stageId) noexcept;
	void PlayEventSound(GameEventType eventType, PegType pegType);
	bool StartStage(std::string_view stageId);
	bool StartSelectedStage();
	void BeginNewRun();
	bool SaveRunCheckpoint();
	bool RestoreRunCheckpoint(const RunCheckpoint& checkpoint);
	bool SelectRunReward(std::size_t index);
	bool PurchaseShopOffer(std::size_t index);
	bool LeaveShop();
	void SaveOptions();
	void RequestSelectiveReset(bool resetSettings);
	void ApplyAudioOptions();
	void UpdateScreenMusic();
	void SetScreenMode(ScreenMode mode);
	void PollGamepad(float deltaSeconds);
	void ApplyGamepadRumble(float deltaSeconds);
	void StopGamepadRumble() noexcept;
	void SetDemoMode(bool enabled);
	void UpdateDemoRun(float deltaSeconds);
	UiScreenKind CurrentUiScreen() const noexcept;
	std::size_t VisibleStageCount() const noexcept;
	void RecordResult(bool cleared);
	void ReloadLocalization();
	bool ReloadContentPreview();
	void SetContentPreview(bool enabled);
	CString Text(std::string_view key) const;
	CString DifficultyTextForUi(GameDifficulty difficulty) const;
	CString PegColorModeTextForUi(PegColorMode colorMode) const;

	Background _background;
	GameWorld _game;
	UINT_PTR _gameTimerId = 0;
	std::chrono::steady_clock::time_point _lastFrameTime{};
	double _accumulatedTimeSeconds = 0.0;
	std::vector<FeedbackAnimation> _feedbackAnimations;
	std::vector<AttackAnimation> _attackAnimations;
	std::vector<OrbTrailPoint> _orbTrail;
	CombatLog _combatLog{ 12 };
	std::optional<Vector2> _pointerLogical;
	bool _combatLogVisible = false;
	UiAnimationTimeline _screenTransition;
	float _damageFlashSeconds = 0.0f;
	DWORD _previousGamepadButtons = 0;
	bool _gamepadTriggerDown = false;
	bool _gamepadStickLatched = false;
	bool _gamepadConnected = false;
	GamepadConnectionTracker _gamepadConnection;
	GamepadRumbleEnvelope _gamepadRumble;
	unsigned short _appliedLeftMotor = 0;
	unsigned short _appliedRightMotor = 0;
	std::size_t _gamepadFocusIndex = 0;
	Vector2 _gamepadAimDirection{ 0.0f, -1.0f };
	TerminalTransitionGate _terminalTransition;
	float _gameplayVisualTimeSeconds = 0.0f;
	float _orbTrailSampleSeconds = 0.0f;
	GameOptions _options;
	AudioCatalogLoadResult _audioCatalog;
	AudioPlayer _audioPlayer;
	DemoRunController _demoRun;
	bool _demoRequested = false;
	PerformanceMonitor _performanceMonitor;
	bool _performanceHudVisible = false;
	GameSettingsStore _settingsStore;
	GameRecordStore _recordStore;
	RunCheckpointStore _runCheckpointStore;
	GameRecordBook _records;
	ContentLoadResult _contentCatalog;
	GameplayCatalogLoadResult _gameplayCatalog;
	DifficultyCurveAnalysis _contentDifficulty;
	bool _contentPreviewActive = false;
	bool _contentPreviewRequested = false;
	std::size_t _contentPreviewIndex = 0;
	float _contentPreviewTimeSeconds = 0.0f;
	CString _contentPreviewNotice;
	LocalizationLoadResult _localization;
	AdventureRun _run;
	GameDifficulty _runDifficulty = GameDifficulty::Normal;
	float _runPlayerHealth = 0.0f;
	CBitmap _uiBackground;
	bool _uiBackgroundLoaded = false;
	CBitmap _gameplayBackground;
	CBitmap _playerSprite;
	CBitmap _enemySprite;
	CBitmap _enemyBatSprite;
	CBitmap _enemyShamanSprite;
	CBitmap _enemyWolfSprite;
	CBitmap _enemyWispSprite;
	CBitmap _enemyCinderBeetleSprite;
	CBitmap _enemyRootLancerSprite;
	CBitmap _orbSprite;
	CBitmap _orbTravelerIcon;
	CBitmap _orbIronIcon;
	CBitmap _orbEchoIcon;
	CBitmap _orbCinderIcon;
	CBitmap _orbVerdantIcon;
	CBitmap _relicComboLanternIcon;
	CBitmap _relicThornCharmIcon;
	CBitmap _relicBarkGuardIcon;
	CBitmap _relicEmberHeartIcon;
	CBitmap _relicWayfinderIcon;
	CBitmap _shopMerchantSprite;
	CBitmap _stagePreviewForest;
	CBitmap _stagePreviewCrystal;
	CBitmap _stagePreviewFungal;
	CBitmap _stagePreviewEmber;
	CBitmap _stagePreviewCitadel;
	CString _loadoutNotice;
	CString _runNotice;
	CString _optionsNotice;
	std::optional<RunReward> _acquiredReward;
	float _rewardAcquisitionSeconds = 0.0f;
	std::array<bool, 3> _shopPurchased{};
	bool _settingsSaveFailed = false;
	bool _recordSaveFailed = false;
	bool _checkpointSaveFailed = false;
	bool _checkpointWritesEnabled = true;
	ResetConfirmation _resetConfirmation = ResetConfirmation::None;
	float _resetConfirmationSeconds = 0.0f;
	StatisticsDifficultyFilter _statisticsDifficulty = StatisticsDifficultyFilter::All;
	StatisticsSortMode _statisticsSort = StatisticsSortMode::HighScore;
	ScreenMode _screenMode = ScreenMode::StageSelection;
	std::optional<GameResultSummary> _resultSummary;
};

