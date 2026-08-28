#include "pch.h"
#include "TargetBall.h"

void TargetBall::draw(CDC* pDC)
{
	const int savedDc = pDC->SaveDC();

	CBrush brush(RGB(255, 0, 0));
	pDC->SelectObject(&brush);
	pDC->SelectObject(GetStockObject(NULL_PEN));
	pDC->Ellipse(x - size, y - size, x + size, y + size);

	pDC->RestoreDC(savedDc);
}

void TargetBall::setting(float px, float py)
{
	x = px;
	y = py;
}
