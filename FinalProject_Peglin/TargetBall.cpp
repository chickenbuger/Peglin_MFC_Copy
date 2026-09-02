#include "pch.h"
#include "TargetBall.h"
#include <cmath>

void TargetBall::draw(CDC* pDC, PegColorMode colorMode)
{
	const int savedDc = pDC->SaveDC();
	const int centerX = static_cast<int>(std::lround(position.x));
	const int centerY = static_cast<int>(std::lround(position.y));

	if (IsMoving())
	{
		const int motionDc = pDC->SaveDC();
		CPen motionPen(PS_SOLID, 2, RGB(118, 205, 255));
		pDC->SelectObject(&motionPen);
		if (_motion.kind == PegMotionKind::Horizontal)
		{
			pDC->MoveTo(centerX - 19, centerY);
			pDC->LineTo(centerX - 14, centerY);
			pDC->MoveTo(centerX + 14, centerY);
			pDC->LineTo(centerX + 19, centerY);
		}
		else
		{
			pDC->MoveTo(centerX, centerY - 19);
			pDC->LineTo(centerX, centerY - 14);
			pDC->MoveTo(centerX, centerY + 14);
			pDC->LineTo(centerX, centerY + 19);
		}
		pDC->RestoreDC(motionDc);
	}

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
	const CRect outerBounds(
		static_cast<int>(std::lround(position.x - size - 2.0f)),
		static_cast<int>(std::lround(position.y - size - 2.0f)),
		static_cast<int>(std::lround(position.x + size + 2.0f)),
		static_cast<int>(std::lround(position.y + size + 2.0f)));
	CRect coreBounds(
		static_cast<int>(std::lround(position.x - size)),
		static_cast<int>(std::lround(position.y - size)),
		static_cast<int>(std::lround(position.x + size)),
		static_cast<int>(std::lround(position.y + size)));
	{
		const int shapeDc = pDC->SaveDC();
		CPen outlinePen(PS_SOLID, 2, RGB(18, 20, 22));
		CBrush rimBrush(RGB(116, 108, 94));
		pDC->SelectObject(&outlinePen);
		pDC->SelectObject(&rimBrush);
		pDC->Ellipse(outerBounds);

		coreBounds.DeflateRect(2, 2);
		CPen corePen(PS_SOLID, 1, RGB(225, 215, 185));
		CBrush coreBrush(RGB(visual.red, visual.green, visual.blue));
		pDC->SelectObject(&corePen);
		pDC->SelectObject(&coreBrush);
		pDC->Ellipse(coreBounds);

		CBrush highlightBrush(RGB(248, 239, 205));
		pDC->SelectObject(&highlightBrush);
		pDC->SelectStockObject(NULL_PEN);
		const int glintRadius = (std::max)(2, static_cast<int>(std::lround(size * 0.22f)));
		pDC->Ellipse(
			centerX - glintRadius - 3,
			centerY - glintRadius - 3,
			centerX + glintRadius - 3,
			centerY + glintRadius - 3);
		pDC->RestoreDC(shapeDc);
	}

	if (colorMode == PegColorMode::HighContrast || type != PegType::Normal)
	{
		const int markerDc = pDC->SaveDC();
		const int markerRadius = static_cast<int>(std::lround(size * 0.55f));
		COLORREF markerColor = RGB(255, 250, 225);
		if (type == PegType::Bomb)
		{
			markerColor = colorMode == PegColorMode::HighContrast
				? RGB(0, 0, 0)
				: RGB(255, 150, 52);
		}
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
			pDC->MoveTo(centerX - markerRadius, centerY);
			pDC->LineTo(centerX + markerRadius, centerY);
			pDC->MoveTo(centerX, centerY - markerRadius);
			pDC->LineTo(centerX, centerY + markerRadius);
			break;
		}
		pDC->RestoreDC(markerDc);
	}

	pDC->RestoreDC(savedDc);
}

void TargetBall::setting(const PegDefinition& definition, std::size_t sourceIndex)
{
	_origin = definition.position;
	type = definition.type;
	_motion = definition.motion;
	_sourceIndex = sourceIndex;
	UpdateMotion(0.0f);
}

void TargetBall::UpdateMotion(float elapsedSeconds) noexcept
{
	position = PositionAt(elapsedSeconds);
}

Vector2 TargetBall::PositionAt(float elapsedSeconds) const noexcept
{
	if (!_motion.IsMoving())
	{
		return _origin;
	}

	const float offset = std::sin(
		_motion.phase + _motion.angularSpeed * elapsedSeconds) * _motion.amplitude;
	if (_motion.kind == PegMotionKind::Horizontal)
	{
		return { _origin.x + offset, _origin.y };
	}
	return { _origin.x, _origin.y + offset };
}
