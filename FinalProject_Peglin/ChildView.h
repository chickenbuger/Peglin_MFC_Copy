
// ChildView.h: CChildView 클래스의 인터페이스
//


#pragma once

#include <chrono>
#include <optional>
#include <string_view>
#include <vector>

#include "Background.h"
#include "ContentCatalog.h"
#include "GameRecordStore.h"
#include "GameSettingsStore.h"
#include "GameplayCatalog.h"
#include "GameWorld.h"
#include "Localization.h"
#include "RunProgression.h"
#include "UiNavigation.h"
#include "UiRenderer.h"

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
		Playing,
		Reward,
		Result
	};

	struct FeedbackAnimation
	{
		CString text;
		Vector2 position;
		COLORREF color = RGB(255, 255, 255);
		float ageSeconds = 0.0f;
		float lifetimeSeconds = 0.9f;
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
	void UpdateGameStep(float deltaSeconds);
	void ConsumeGameEvents();
	void UpdateFeedbackAnimations(float deltaSeconds);
	void UpdateAttackAnimations(float deltaSeconds);
	void UpdateOrbVisuals(float deltaSeconds);
	void DrawFeedbackAnimations(CDC* deviceContext);
	void DrawAttackAnimations(CDC* deviceContext);
	void DrawOrbTrail(CDC* deviceContext);
	void DrawAimPreview(CDC* deviceContext);
	void DrawEnemyHealthBar(
		CDC* deviceContext,
		const EnemyCombatant& combatant,
		Vector2 drawSize,
		float drawY,
		bool enemyGroup,
		bool activeTarget);
	void DrawStageSelection(CDC* deviceContext);
	void DrawLoadoutScreen(CDC* deviceContext);
	void DrawOptions(CDC* deviceContext);
	void DrawRewardScreen(CDC* deviceContext);
	void DrawResultScreen(CDC* deviceContext);
	void DrawMenuBackdrop(CDC* deviceContext);
	void DrawPlayingLoadout(CDC* deviceContext);
	bool HandleMenuClick(CPoint point);
	void ExecuteUiAction(const UiAction& action);
	CBitmap* GetEnemySprite(EnemyVisualKind visual) noexcept;
	void PlayEventSound(GameEventType eventType, PegType pegType);
	bool StartStage(std::string_view stageId);
	bool StartSelectedStage();
	void BeginNewRun();
	bool SelectRunReward(std::size_t index);
	void SaveOptions();
	void RecordResult(bool cleared);
	void ReloadLocalization();
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
	float _gameplayVisualTimeSeconds = 0.0f;
	float _orbTrailSampleSeconds = 0.0f;
	GameOptions _options;
	GameSettingsStore _settingsStore;
	GameRecordStore _recordStore;
	GameRecordBook _records;
	ContentLoadResult _contentCatalog;
	GameplayCatalogLoadResult _gameplayCatalog;
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
	CBitmap _orbSprite;
	CString _loadoutNotice;
	CString _runNotice;
	bool _settingsSaveFailed = false;
	bool _recordSaveFailed = false;
	ScreenMode _screenMode = ScreenMode::StageSelection;
	std::optional<GameResultSummary> _resultSummary;
};

