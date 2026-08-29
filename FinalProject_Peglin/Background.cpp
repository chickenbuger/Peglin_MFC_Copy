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

void Background::draw(CDC* pDC)
{
	const int savedDc = pDC->SaveDC();

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
