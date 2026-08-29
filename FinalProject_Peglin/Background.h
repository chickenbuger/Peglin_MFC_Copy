#pragma once
class Background
{
public:
	Background() = default;
	~Background() {}
public:
	void draw(CDC* pDC, CBitmap* gameplayBackground = nullptr);
};
