#include "pch.h"
#include "Player.h"

void Player::draw(CDC* pDC)
{
	if (hp > 0.0f)
	{
		CBitmap bmp;
		bmp.LoadBitmap(312);

		CBrush brush(&bmp);
		pDC->SelectObject(brush);
		pDC->SelectObject(GetStockObject(NULL_PEN));
		pDC->Rectangle(130, 130, 190, 190);
	}
}

void Player::Init()
{
	hp = 100.0f;
}
