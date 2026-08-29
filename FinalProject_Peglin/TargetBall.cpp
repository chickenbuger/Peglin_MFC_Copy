#include "pch.h"
#include "TargetBall.h"
#include <cmath>

void TargetBall::draw(CDC* pDC, PegColorMode colorMode)
{
	const int savedDc = pDC->SaveDC();

	PegVisualStyle visual = GetPegTypeDefinition(type).visual;
	if (colorMode == PegColorMode::HighContrast)
	{
		switch (type)
		{
		case PegType::Normal: visual = { 30, 130, 255 }; break;
		case PegType::Critical: visual = { 255, 220, 0 }; break;
		case PegType::Bomb: visual = { 245, 245, 245 }; break;
		case PegType::Refresh: visual = { 0, 255, 180 }; break;
		}
	}
	CBrush brush(RGB(visual.red, visual.green, visual.blue));
	pDC->SelectObject(&brush);
	pDC->SelectObject(GetStockObject(NULL_PEN));
	pDC->Ellipse(
		static_cast<int>(std::lround(position.x - size)),
		static_cast<int>(std::lround(position.y - size)),
		static_cast<int>(std::lround(position.x + size)),
		static_cast<int>(std::lround(position.y + size)));

	if (colorMode == PegColorMode::HighContrast)
	{
		const int centerX = static_cast<int>(std::lround(position.x));
		const int centerY = static_cast<int>(std::lround(position.y));
		const int markerRadius = static_cast<int>(std::lround(size * 0.55f));
		const COLORREF markerColor = type == PegType::Bomb
			? RGB(0, 0, 0)
			: RGB(255, 255, 255);
		CPen markerPen(PS_SOLID, 2, markerColor);
		pDC->SelectObject(&markerPen);
		pDC->SelectObject(GetStockObject(NULL_BRUSH));
		switch (type)
		{
		case PegType::Normal:
			pDC->MoveTo(centerX - markerRadius, centerY);
			pDC->LineTo(centerX + markerRadius, centerY);
			break;
		case PegType::Critical:
			pDC->MoveTo(centerX - markerRadius, centerY);
			pDC->LineTo(centerX + markerRadius, centerY);
			pDC->MoveTo(centerX, centerY - markerRadius);
			pDC->LineTo(centerX, centerY + markerRadius);
			break;
		case PegType::Bomb:
			pDC->MoveTo(centerX - markerRadius, centerY - markerRadius);
			pDC->LineTo(centerX + markerRadius, centerY + markerRadius);
			pDC->MoveTo(centerX + markerRadius, centerY - markerRadius);
			pDC->LineTo(centerX - markerRadius, centerY + markerRadius);
			break;
		case PegType::Refresh:
			pDC->Ellipse(
				centerX - markerRadius,
				centerY - markerRadius,
				centerX + markerRadius,
				centerY + markerRadius);
			break;
		}
	}

	pDC->RestoreDC(savedDc);
}

void TargetBall::setting(const PegDefinition& definition)
{
	position = definition.position;
	type = definition.type;
}
