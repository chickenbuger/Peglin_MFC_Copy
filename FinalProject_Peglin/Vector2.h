#pragma once

#include <cmath>

struct Vector2
{
	float x = 0.0f;
	float y = 0.0f;

	constexpr Vector2() noexcept = default;
	constexpr Vector2(float xValue, float yValue) noexcept : x(xValue), y(yValue) {}

	constexpr Vector2 operator+(const Vector2& other) const noexcept
	{
		return { x + other.x, y + other.y };
	}

	constexpr Vector2 operator-(const Vector2& other) const noexcept
	{
		return { x - other.x, y - other.y };
	}

	constexpr Vector2 operator*(float scalar) const noexcept
	{
		return { x * scalar, y * scalar };
	}

	constexpr Vector2 operator/(float scalar) const noexcept
	{
		return { x / scalar, y / scalar };
	}

	constexpr Vector2& operator+=(const Vector2& other) noexcept
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	constexpr float LengthSquared() const noexcept
	{
		return x * x + y * y;
	}

	float Length() const noexcept
	{
		return std::sqrt(LengthSquared());
	}

	Vector2 Normalized(float epsilon = 0.000001f) const noexcept
	{
		const float length = Length();
		return length <= epsilon ? Vector2{} : *this / length;
	}
};

constexpr float Dot(const Vector2& left, const Vector2& right) noexcept
{
	return left.x * right.x + left.y * right.y;
}
