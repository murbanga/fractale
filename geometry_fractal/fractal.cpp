#include "fractal.h"

double distance(const Point &pt, double x, double y)
{
	return sqrt((pt.x - x) * (pt.x - x) + (pt.y - y) * (pt.y - y));
}

float distance(const Point &a, const Point &b)
{
	return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

Point operator - (const Point &a, const Point &b)
{
	return { a.x - b.x,a.y - b.y };
}

Point operator + (const Point &a, const Point &b)
{
	return { a.x + b.x,a.y + b.y };
}

Point operator *(const Point &a, float b)
{
	return { a.x * b,a.y * b };
}

Point rotate(const Point &a, float angle)
{
	return {
		a.x * cosf(angle) - a.y * sinf(angle),
		a.x * sinf(angle) + a.y * cosf(angle) };
}

std::pair<Point, int> Fractal::nearest(const Point &p) const
{
	int idx = -1;
	float dist = FLT_MAX;
	for (int i = 0; i < model.size(); ++i)
	{
		float d = (model[i].x - p.x) * (model[i].x - p.x) + (model[i].y - p.y) * (model[i].y - p.y);
		if (d < dist)
		{
			dist = d;
			idx = i;
		}
	}
	if (idx >= 0)
		return { model[idx],idx };
	else
		return { {},-1 };
}

void Fractal::clear()
{
	model.clear();
	current.clear();
	iterations = 0;
}

Fractal &Fractal::operator++()
{
	if(current.empty())
	{
		current = model;
	}

	size_t m = model.size();
	size_t n = current.size();

	std::vector<Point> next;
	next.reserve((n - 1) * (model.size()-1) + 1);
	next.push_back(current[0]);

	Point ma = model[0];
	Point mb = model[m - 1];

	float model_length = distance(ma, mb);
	float model_angle = atan2f(mb.y - ma.y, mb.x - ma.x);

	for (size_t i = 0; i < n - 1; ++i)
	{
		Point a = current[i];
		Point b = current[i + 1];

		float scale = distance(a, b) / model_length;
		float angle = atan2f(b.y - a.y, b.x - a.x) - model_angle;
		float cos_a = cosf(angle);
		float sin_a = sinf(angle);

		for (size_t j = 1; j < m; ++j)
		{
			Point c = (model[j] - ma) * scale;
			Point d = {
				c.x * cos_a - c.y * sin_a + a.x,
				c.x * sin_a + c.y * cos_a + a.y };
			next.push_back(d);
		}
	}

	current = std::move(next);
	++iterations;

	return *this;
}