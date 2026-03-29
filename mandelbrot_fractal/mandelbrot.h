#pragma once

template <typename T> struct Rect {
	T x0;
	T x1;
	T y0;
	T y1;

	Rect<T> normalize() const
	{
		return { min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1) };
	}

	bool valid() const { return x0 >= 0 && y0 >= 0 && x1 >= 0 && y1 >= 0; }
};

struct Image {
	uint8_t *buf = nullptr;
	size_t buf_size = 0;
	int width = 0;
	int height = 0;
	int idx = 0;
};

template <typename T>
int mandelbrot(Image &image, int left, int top, int width, int height, const Rect<T> &r, int n,
               const uint32_t *palette);
