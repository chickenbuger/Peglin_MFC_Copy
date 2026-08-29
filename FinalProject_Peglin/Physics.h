#pragma once

#include "Vector2.h"

constexpr float ClampRestitution(float restitution) noexcept
{
	return restitution < 0.0f ? 0.0f : (restitution > 1.0f ? 1.0f : restitution);
}

constexpr Vector2 ReflectVelocity(
	const Vector2& velocity,
	const Vector2& unitNormal,
	float restitution) noexcept
{
	const float normalSpeed = Dot(velocity, unitNormal);
	if (normalSpeed >= 0.0f)
	{
		return velocity;
	}

	return velocity - unitNormal * ((1.0f + ClampRestitution(restitution)) * normalSpeed);
}

static_assert(ReflectVelocity({ -2.0f, 3.0f }, { 1.0f, 0.0f }, 0.5f).x == 1.0f);
static_assert(ReflectVelocity({ -2.0f, 3.0f }, { 1.0f, 0.0f }, 0.5f).y == 3.0f);
static_assert(ReflectVelocity({ 2.0f, 3.0f }, { 1.0f, 0.0f }, 0.5f).x == 2.0f);
