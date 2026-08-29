#include "pch.h"
#include "UiRenderer.h"

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

void UiRenderer::DrawPanel(
	CDC* deviceContext,
	const CRect& bounds,
	bool selected,
	COLORREF accent)
{
	const int savedDc = deviceContext->SaveDC();
	CPen borderPen(PS_SOLID, selected ? 3 : 1, selected ? accent : UiTheme::Border);
	CBrush panelBrush(selected ? UiTheme::PanelMuted : UiTheme::Panel);
	deviceContext->SelectObject(&borderPen);
	deviceContext->SelectObject(&panelBrush);
	deviceContext->RoundRect(bounds, CPoint(16, 16));
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
