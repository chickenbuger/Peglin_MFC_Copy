#include "pch.h"
#include "Enemy.h"
#include <cmath>

void Enemy::draw(CDC* pDC, CBitmap* sprite)
{
	const int savedDc = pDC->SaveDC();

	if (hp >= 0.0f)
	{
		const int enemyX = static_cast<int>(std::lround(x));
		const int enemyY = static_cast<int>(std::lround(GameLayout::EnemyInitialPosition.y));
		const int width = static_cast<int>(std::lround(GameLayout::EnemySize.x));
		const int height = static_cast<int>(std::lround(GameLayout::EnemySize.y));
		if (sprite != nullptr && sprite->GetSafeHandle() != nullptr)
		{
			CDC spriteDc;
			spriteDc.CreateCompatibleDC(pDC);
			CBitmap* previousBitmap = spriteDc.SelectObject(sprite);
			BITMAP bitmapInfo{};
			sprite->GetBitmap(&bitmapInfo);
			::TransparentBlt(
				pDC->GetSafeHdc(), enemyX, enemyY, width, height,
				spriteDc.GetSafeHdc(), 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight,
				RGB(255, 0, 255));
			spriteDc.SelectObject(previousBitmap);
			pDC->RestoreDC(savedDc);
			return;
		}

		CBitmap bmp;
		bmp.LoadBitmap(313);

		CBrush brush(&bmp);
		pDC->SelectObject(&brush);
		pDC->SelectObject(GetStockObject(NULL_PEN));
		pDC->Rectangle(
			enemyX,
			enemyY,
			enemyX + width,
			enemyY + height);
	}

	pDC->RestoreDC(savedDc);
}

void Enemy::Init()
{
	x = GameLayout::EnemyInitialPosition.x;
	hp = 20.0f;
	count = 0;
}
