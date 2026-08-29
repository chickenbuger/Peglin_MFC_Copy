#include "pch.h"
#include "Player.h"
#include "GameLayout.h"

#include <cmath>

void Player::draw(CDC* pDC)
{
	const int savedDc = pDC->SaveDC();

	if (hp > 0.0f)
	{
		CBitmap bmp;
		bmp.LoadBitmap(312);

		CBrush brush(&bmp);
		pDC->SelectObject(&brush);
		pDC->SelectObject(GetStockObject(NULL_PEN));
		const int left = static_cast<int>(std::lround(GameLayout::PlayerPosition.x));
		const int top = static_cast<int>(std::lround(GameLayout::PlayerPosition.y));
		pDC->Rectangle(
			left,
			top,
			left + static_cast<int>(std::lround(GameLayout::PlayerSize.x)),
			top + static_cast<int>(std::lround(GameLayout::PlayerSize.y)));
	}

	pDC->RestoreDC(savedDc);
}

void Player::Init()
{
	hp = 100.0f;
}
