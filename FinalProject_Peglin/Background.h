#pragma once
class Background
{
private:
	float x1, x2;
	float y1, y2;

public:
	Background() { x1 = 20; y1 = 200; x2 = 960; y2 = 700; }
	~Background() {}
public:
	void draw(CDC* pDC);
};