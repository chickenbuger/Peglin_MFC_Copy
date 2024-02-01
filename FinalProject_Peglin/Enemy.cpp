#include "pch.h"
#include "Enemy.h"

void Enemy::draw(CDC* pDC)
{
	if (hp >= 0.0f)
	{
		CBitmap bmp;
		bmp.LoadBitmap(313);

		CBrush brush(&bmp);
		pDC->SelectObject(brush);
		pDC->SelectObject(GetStockObject(NULL_PEN));
		pDC->Rectangle(x, 130, x + 60, 190);
	}
}

void Enemy::Init()
{
	x = Init_x;
	hp = 20.0f;
	count = 0;
}
