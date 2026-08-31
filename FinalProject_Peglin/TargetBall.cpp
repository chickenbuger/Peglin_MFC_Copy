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
