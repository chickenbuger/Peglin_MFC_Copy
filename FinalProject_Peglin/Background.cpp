#include "pch.h"
#include "Background.h"

void Background::draw(CDC* pDC)
{
	CBitmap bmp;
	bmp.LoadBitmap(311);

	CBrush brush(&bmp);
	pDC->SelectObject(brush);
	pDC->Rectangle(0, 0, 980, 200);

	CPen pen(PS_SOLID, 4, RGB(255.0f, 255.0f, 255.0f));
	pDC->SelectObject(pen);

	pDC->MoveTo(x1, y1);
	pDC->LineTo(x2, y1);

	pDC->MoveTo(x1, y1);
	pDC->LineTo(x1, y2);
	
	pDC->MoveTo(x2, y1);
	pDC->LineTo(x2, y2);
	
	CBrush brush1(RGB(255.0, 255.0, 255.0));
	pDC->SelectObject(brush1);

	pDC->Rectangle(0, y1, x1, y2);
	pDC->Rectangle(x2, y1, 1000, y2);
}