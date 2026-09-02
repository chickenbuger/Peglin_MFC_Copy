#include "pch.h"
#include "UiRenderer.h"

#include <algorithm>
#include <cmath>

namespace
{
	COLORREF BlendColor(COLORREF from, COLORREF to, float amount) noexcept
	{
		const float blend = std::clamp(amount, 0.0f, 1.0f);
		const auto channel = [blend](BYTE start, BYTE finish)
		{
			return static_cast<BYTE>(std::lround(
				static_cast<float>(start)
				+ (static_cast<float>(finish) - static_cast<float>(start)) * blend));
		};
		return RGB(
			channel(GetRValue(from), GetRValue(to)),
			channel(GetGValue(from), GetGValue(to)),
			channel(GetBValue(from), GetBValue(to)));
	}
}

void UiRenderer::DrawBackdrop(CDC* deviceContext, CBitmap* bitmap, const CRect& bounds)
{
	deviceContext->FillSolidRect(bounds, UiTheme::Canvas);
	if (bitmap == nullptr || bitmap->GetSafeHandle() == nullptr)
	{
		return;
	}

	BITMAP bitmapInfo{};
	if (bitmap->GetBitmap(&bitmapInfo) == 0 || bitmapInfo.bmWidth <= 0 || bitmapInfo.bmHeight <= 0)
	{
		return;
	}

	CDC source;
	if (!source.CreateCompatibleDC(deviceContext))
	{
		return;
	}
	CBitmap* previousBitmap = source.SelectObject(bitmap);
	const int savedDc = deviceContext->SaveDC();
	deviceContext->SetStretchBltMode(HALFTONE);
	deviceContext->SetBrushOrg(0, 0);
	deviceContext->StretchBlt(
		bounds.left,
		bounds.top,
		bounds.Width(),
		bounds.Height(),
		&source,
		0,
		0,
		bitmapInfo.bmWidth,
		bitmapInfo.bmHeight,
		SRCCOPY);
	deviceContext->RestoreDC(savedDc);
	source.SelectObject(previousBitmap);
}

bool UiRenderer::DrawTransparentBitmap(
	CDC* deviceContext,
	CBitmap* bitmap,
	const CRect& bounds,
	COLORREF transparentColor)
{
	if (deviceContext == nullptr
		|| bitmap == nullptr
		|| bitmap->GetSafeHandle() == nullptr
		|| bounds.IsRectEmpty())
	{
		return false;
	}

	BITMAP bitmapInfo{};
	if (bitmap->GetBitmap(&bitmapInfo) == 0
		|| bitmapInfo.bmWidth <= 0
		|| bitmapInfo.bmHeight <= 0)
	{
		return false;
	}

	CDC source;
	if (!source.CreateCompatibleDC(deviceContext))
	{
		return false;
	}
	CBitmap* previousBitmap = source.SelectObject(bitmap);
	const int savedDc = deviceContext->SaveDC();
	deviceContext->SetStretchBltMode(HALFTONE);
	deviceContext->SetBrushOrg(0, 0);
	const BOOL drawn = deviceContext->TransparentBlt(
		bounds.left,
		bounds.top,
		bounds.Width(),
		bounds.Height(),
		&source,
		0,
		0,
		bitmapInfo.bmWidth,
		bitmapInfo.bmHeight,
		transparentColor);
	deviceContext->RestoreDC(savedDc);
	source.SelectObject(previousBitmap);
	return drawn != FALSE;
}

void UiRenderer::DrawPanel(
	CDC* deviceContext,
	const CRect& bounds,
	bool selected,
	COLORREF accent)
{
	const int savedDc = deviceContext->SaveDC();
	const COLORREF borderColor = selected ? accent : UiTheme::Border;
	CPen borderPen(PS_SOLID, selected ? 3 : 2, borderColor);
	CBrush panelBrush(selected ? UiTheme::PanelMuted : UiTheme::Panel);
	deviceContext->SelectObject(&borderPen);
	deviceContext->SelectObject(&panelBrush);
	deviceContext->RoundRect(bounds, CPoint(8, 8));

	CRect inset(bounds);
	inset.DeflateRect(4, 4);
	CPen insetPen(PS_SOLID, 1, BlendColor(borderColor, UiTheme::Panel, 0.45f));
	deviceContext->SelectObject(&insetPen);
	deviceContext->SelectStockObject(NULL_BRUSH);
	deviceContext->RoundRect(inset, CPoint(5, 5));

	const COLORREF bevelLight = BlendColor(borderColor, RGB(255, 255, 255), 0.24f);
	const COLORREF bevelShadow = RGB(6, 10, 17);
	CPen lightPen(PS_SOLID, 1, bevelLight);
	deviceContext->SelectObject(&lightPen);
	deviceContext->MoveTo(bounds.left + 7, bounds.top + 4);
	deviceContext->LineTo(bounds.right - 7, bounds.top + 4);
	deviceContext->MoveTo(bounds.left + 4, bounds.top + 7);
	deviceContext->LineTo(bounds.left + 4, bounds.bottom - 7);
	CPen shadowPen(PS_SOLID, 2, bevelShadow);
	deviceContext->SelectObject(&shadowPen);
	deviceContext->MoveTo(bounds.left + 7, bounds.bottom - 4);
	deviceContext->LineTo(bounds.right - 7, bounds.bottom - 4);
	deviceContext->MoveTo(bounds.right - 4, bounds.top + 7);
	deviceContext->LineTo(bounds.right - 4, bounds.bottom - 7);
	deviceContext->RestoreDC(savedDc);
}

void UiRenderer::DrawText(
	CDC* deviceContext,
	const CRect& bounds,
	const CString& text,
	int pointSize,
	COLORREF color,
	UINT format)
{
	const int savedDc = deviceContext->SaveDC();
	CFont font;
	font.CreatePointFont(pointSize, _T("Malgun Gothic"));
	deviceContext->SelectObject(&font);
	deviceContext->SetBkMode(TRANSPARENT);
	deviceContext->SetTextColor(color);
	CRect textBounds(bounds);
	deviceContext->DrawText(text, textBounds, format);
	deviceContext->RestoreDC(savedDc);
}

void UiRenderer::DrawKeyHint(
	CDC* deviceContext,
	const CRect& bounds,
	const CString& text)
{
	DrawPanel(deviceContext, bounds, false, UiTheme::Blue);
	DrawText(deviceContext, bounds, text, 125, UiTheme::Blue);
}

void UiRenderer::DrawProgressBar(
	CDC* deviceContext,
	const CRect& bounds,
	float normalizedValue,
	const CString& text,
	COLORREF fillColor,
	COLORREF borderColor)
{
	const int savedDc = deviceContext->SaveDC();
	const float fraction = std::clamp(normalizedValue, 0.0f, 1.0f);
	deviceContext->FillSolidRect(bounds, RGB(8, 14, 24));

	CRect fillBounds(bounds);
	fillBounds.DeflateRect(2, 2);
	fillBounds.right = fillBounds.left + static_cast<int>(std::lround(
		static_cast<float>(fillBounds.Width()) * fraction));
	if (fillBounds.right > fillBounds.left)
	{
		deviceContext->FillSolidRect(fillBounds, fillColor);
		CRect highlight(fillBounds);
		highlight.bottom = (std::min)(highlight.bottom, highlight.top + 3);
		deviceContext->FillSolidRect(
			highlight,
			BlendColor(fillColor, RGB(255, 255, 255), 0.28f));
	}

	CPen dividerPen(PS_SOLID, 1, RGB(28, 35, 42));
	deviceContext->SelectObject(&dividerPen);
	for (int segment = 1; segment < 10; ++segment)
	{
		const int x = bounds.left + static_cast<int>(std::lround(
			static_cast<float>(bounds.Width()) * static_cast<float>(segment) / 10.0f));
		deviceContext->MoveTo(x, bounds.top + 2);
		deviceContext->LineTo(x, bounds.bottom - 2);
	}

	CPen borderPen(PS_SOLID, 2, borderColor);
	deviceContext->SelectObject(&borderPen);
	deviceContext->SelectStockObject(NULL_BRUSH);
	deviceContext->Rectangle(bounds);

	CFont font;
	if (!text.IsEmpty())
	{
		font.CreatePointFont(70, _T("Malgun Gothic"));
		deviceContext->SelectObject(&font);
		deviceContext->SetBkMode(TRANSPARENT);
		deviceContext->SetTextColor(UiTheme::Text);
		CRect textBounds(bounds);
		deviceContext->DrawText(
			text,
			textBounds,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	deviceContext->RestoreDC(savedDc);
}
