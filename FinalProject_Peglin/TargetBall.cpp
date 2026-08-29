#include "pch.h"
#include "TargetBall.h"
#include <cmath>

void TargetBall::draw(CDC* pDC)
{
	const int savedDc = pDC->SaveDC();

	const PegVisualStyle visual = GetPegTypeDefinition(type).visual;
	CBrush brush(RGB(visual.red, visual.green, visual.blue));
	pDC->SelectObject(&brush);
	pDC->SelectObject(GetStockObject(NULL_PEN));
	pDC->Ellipse(
		static_cast<int>(std::lround(position.x - size)),
		static_cast<int>(std::lround(position.y - size)),
		static_cast<int>(std::lround(position.x + size)),
		static_cast<int>(std::lround(position.y + size)));

	pDC->RestoreDC(savedDc);
}

void TargetBall::setting(const PegDefinition& definition)
{
	position = definition.position;
	type = definition.type;
}
