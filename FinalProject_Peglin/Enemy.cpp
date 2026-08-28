#include "pch.h"
#include "Enemy.h"
#include <cmath>

void Enemy::draw(CDC* pDC)
{
	const int savedDc = pDC->SaveDC();

	if (hp >= 0.0f)
	{
		CBitmap bmp;
		bmp.LoadBitmap(313);

		CBrush brush(&bmp);
		pDC->SelectObject(&brush);
		pDC->SelectObject(GetStockObject(NULL_PEN));
		const int enemyX = static_cast<int>(std::lround(x));
		pDC->Rectangle(enemyX, 130, enemyX + 60, 190);
	}

	pDC->RestoreDC(savedDc);
}

void Enemy::Init()
{
	x = Init_x;
	hp = 20.0f;
	count = 0;
}
