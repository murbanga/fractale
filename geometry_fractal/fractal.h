#pragma once
#include <vector>

struct Point
{
	float x;
	float y;

	bool valid() const { return isfinite(x) && isfinite(y); }
};

double distance(const Point &pt, double x, double y);
float distance(const Point &a, const Point &b);

struct BoundingBox
{
	float x;
	float y;
	float width;
	float height;
};

struct Fractal
{
	void clear();
	Fractal& operator++();
	std::pair<Point, int> nearest(const Point &p) const;

	std::vector<Point> model;
	std::vector<Point> current;
	int iterations = 0;
};