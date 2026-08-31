#pragma once

#include <afxwin.h>

namespace UiTheme
{
	inline constexpr COLORREF Canvas = RGB(12, 18, 30);
	inline constexpr COLORREF Panel = RGB(18, 30, 48);
	inline constexpr COLORREF PanelMuted = RGB(25, 39, 58);
	inline constexpr COLORREF Border = RGB(91, 114, 132);
	inline constexpr COLORREF Gold = RGB(244, 199, 82);
	inline constexpr COLORREF Blue = RGB(113, 190, 235);
	inline constexpr COLORREF Green = RGB(108, 207, 146);
	inline constexpr COLORREF Orange = RGB(245, 154, 79);
	inline constexpr COLORREF Text = RGB(237, 241, 232);
	inline constexpr COLORREF MutedText = RGB(166, 182, 190);
	inline constexpr COLORREF Danger = RGB(244, 112, 112);
}

class UiRenderer
{
public:
	static void DrawBackdrop(CDC* deviceContext, CBitmap* bitmap, const CRect& bounds);
	static bool DrawTransparentBitmap(
		CDC* deviceContext,
		CBitmap* bitmap,
		const CRect& bounds,
		COLORREF transparentColor = RGB(255, 0, 255));
	static void DrawPanel(
		CDC* deviceContext,
		const CRect& bounds,
		bool selected = false,
		COLORREF accent = UiTheme::Gold);
	static void DrawText(
		CDC* deviceContext,
		const CRect& bounds,
		const CString& text,
		int pointSize,
		COLORREF color = UiTheme::Text,
		UINT format = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	static void DrawKeyHint(
		CDC* deviceContext,
		const CRect& bounds,
		const CString& text);
	static void DrawProgressBar(
		CDC* deviceContext,
		const CRect& bounds,
		float normalizedValue,
		const CString& text,
		COLORREF fillColor = UiTheme::Green,
		COLORREF borderColor = UiTheme::Border);
};
