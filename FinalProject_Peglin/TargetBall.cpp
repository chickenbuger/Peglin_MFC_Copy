#include "pch.h"
#include "TargetBall.h"
#include <cmath>

void TargetBall::draw(CDC* pDC)
{
	const int savedDc = pDC->SaveDC();

	CBrush brush(RGB(255, 0, 0));
	pDC->SelectObject(&brush);
	pDC->SelectObject(GetStockObject(NULL_PEN));
	pDC->Ellipse(
		static_cast<int>(std::lround(position.x - size)),
		static_cast<int>(std::lround(position.y - size)),
		static_cast<int>(std::lround(position.x + size)),
		static_cast<int>(std::lround(position.y + size)));

	pDC->RestoreDC(savedDc);
}

void TargetBall::setting(Vector2 newPosition)
{
	position = newPosition;
}
