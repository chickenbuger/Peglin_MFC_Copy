#include "pch.h"
#include "Player.h"
#include "GameLayout.h"

#include <cmath>

void Player::draw(CDC* pDC, CBitmap* sprite)
{
	const int savedDc = pDC->SaveDC();

	if (hp > 0.0f)
	{
		const int left = static_cast<int>(std::lround(GameLayout::PlayerPosition.x));
		const int top = static_cast<int>(std::lround(GameLayout::PlayerPosition.y));
		const int width = static_cast<int>(std::lround(GameLayout::PlayerSize.x));
		const int height = static_cast<int>(std::lround(GameLayout::PlayerSize.y));
		if (sprite != nullptr && sprite->GetSafeHandle() != nullptr)
		{
			CDC spriteDc;
			spriteDc.CreateCompatibleDC(pDC);
			CBitmap* previousBitmap = spriteDc.SelectObject(sprite);
			BITMAP bitmapInfo{};
			sprite->GetBitmap(&bitmapInfo);
			::TransparentBlt(
				pDC->GetSafeHdc(), left, top, width, height,
				spriteDc.GetSafeHdc(), 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight,
				RGB(255, 0, 255));
			spriteDc.SelectObject(previousBitmap);
			pDC->RestoreDC(savedDc);
			return;
		}

		CBitmap bmp;
		bmp.LoadBitmap(312);

		CBrush brush(&bmp);
		pDC->SelectObject(&brush);
		pDC->SelectObject(GetStockObject(NULL_PEN));
		pDC->Rectangle(
			left,
			top,
			left + width,
			top + height);
	}

	pDC->RestoreDC(savedDc);
}

void Player::Init()
{
	hp = 100.0f;
}
