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
		const int enemyY = static_cast<int>(std::lround(GameLayout::EnemyInitialPosition.y));
		pDC->Rectangle(
			enemyX,
			enemyY,
			enemyX + static_cast<int>(std::lround(GameLayout::EnemySize.x)),
			enemyY + static_cast<int>(std::lround(GameLayout::EnemySize.y)));
	}

	pDC->RestoreDC(savedDc);
}

void Enemy::Init()
{
	x = GameLayout::EnemyInitialPosition.x;
	hp = 20.0f;
	count = 0;
}
