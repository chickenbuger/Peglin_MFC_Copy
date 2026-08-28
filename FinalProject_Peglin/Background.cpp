#include "pch.h"
#include "Background.h"
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
	pDC->Rectangle(0, 0, 980, 200);

	CPen pen(PS_SOLID, 4, RGB(255, 255, 255));
	pDC->SelectObject(&pen);

	const int left = RoundToPixel(x1);
	const int top = RoundToPixel(y1);
	const int right = RoundToPixel(x2);
	const int bottom = RoundToPixel(y2);

	pDC->MoveTo(left, top);
	pDC->LineTo(right, top);

	pDC->MoveTo(left, top);
	pDC->LineTo(left, bottom);
	
	pDC->MoveTo(right, top);
	pDC->LineTo(right, bottom);
	
	CBrush brush1(RGB(255, 255, 255));
	pDC->SelectObject(&brush1);

	pDC->Rectangle(0, top, left, bottom);
	pDC->Rectangle(right, top, 1000, bottom);

	pDC->RestoreDC(savedDc);
}
