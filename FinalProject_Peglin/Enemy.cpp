#include "pch.h"
#include "Enemy.h"
#include <cmath>

void Enemy::draw(
	CDC* pDC,
	CBitmap* sprite,
	Vector2 drawSize,
	float drawY,
	bool activeTarget) const
{
	const int savedDc = pDC->SaveDC();

	if (hp > 0.0f)
	{
		const int enemyX = static_cast<int>(std::lround(x));
		const int enemyY = static_cast<int>(std::lround(drawY));
		const int width = static_cast<int>(std::lround(drawSize.x));
		const int height = static_cast<int>(std::lround(drawSize.y));
		if (activeTarget)
		{
			CPen targetPen(PS_SOLID, 3, RGB(255, 194, 62));
			CPen* previousPen = pDC->SelectObject(&targetPen);
			pDC->SelectObject(GetStockObject(NULL_BRUSH));
			pDC->Ellipse(enemyX - 5, enemyY - 5, enemyX + width + 5, enemyY + height + 5);
			pDC->SelectObject(previousPen);
		}
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
		CBrush* previousBrush = pDC->SelectObject(&brush);
		CPen* previousPen = pDC->SelectObject(
			static_cast<CPen*>(CPen::FromHandle(
				static_cast<HPEN>(GetStockObject(NULL_PEN)))));
		pDC->Rectangle(
			enemyX,
			enemyY,
			enemyX + width,
			enemyY + height);
		pDC->SelectObject(previousPen);
		pDC->SelectObject(previousBrush);
	}

	pDC->RestoreDC(savedDc);
}

void Enemy::Init()
{
	x = GameLayout::EnemyInitialPosition.x;
	hp = 20.0f;
	count = 0;
}
