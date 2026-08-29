#include "pch.h"
#include "Background.h"
#include "GameLayout.h"
#include <cmath>

namespace
{
	int RoundToPixel(float value)
	{
		return static_cast<int>(std::lround(value));
	}
}

void Background::draw(CDC* pDC, CBitmap* gameplayBackground)
{
	const int savedDc = pDC->SaveDC();
	if (gameplayBackground != nullptr && gameplayBackground->GetSafeHandle() != nullptr)
	{
		CDC sourceDc;
		sourceDc.CreateCompatibleDC(pDC);
		CBitmap* previousBitmap = sourceDc.SelectObject(gameplayBackground);
		BITMAP bitmapInfo{};
		gameplayBackground->GetBitmap(&bitmapInfo);
		pDC->StretchBlt(
			0,
			0,
			RoundToPixel(GameLayout::WindowWidth),
			RoundToPixel(GameLayout::WindowHeight),
			&sourceDc,
			0,
			0,
			bitmapInfo.bmWidth,
			bitmapInfo.bmHeight,
			SRCCOPY);
		sourceDc.SelectObject(previousBitmap);
		pDC->RestoreDC(savedDc);
		return;
	}

	CBitmap bmp;
	bmp.LoadBitmap(311);

	CBrush brush(&bmp);
	pDC->SelectObject(&brush);
	pDC->Rectangle(0, 0, RoundToPixel(GameLayout::SceneWidth), RoundToPixel(GameLayout::HeaderHeight));

	CPen pen(PS_SOLID, 4, RGB(255, 255, 255));
	pDC->SelectObject(&pen);

	const int left = RoundToPixel(GameLayout::BoardLeft);
	const int top = RoundToPixel(GameLayout::BoardTop);
	const int right = RoundToPixel(GameLayout::BoardRight);
	const int bottom = RoundToPixel(GameLayout::BoardBottom);

	pDC->MoveTo(left, top);
	pDC->LineTo(right, top);

	pDC->MoveTo(left, top);
	pDC->LineTo(left, bottom);
	
	pDC->MoveTo(right, top);
	pDC->LineTo(right, bottom);
	
	CBrush brush1(RGB(255, 255, 255));
	pDC->SelectObject(&brush1);

	pDC->Rectangle(0, top, left, bottom);
	pDC->Rectangle(right, top, RoundToPixel(GameLayout::WindowWidth), bottom);

	pDC->RestoreDC(savedDc);
}
