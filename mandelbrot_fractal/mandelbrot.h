#pragma once

template <typename T> struct rect {
	T x0;
	T x1;
	T y0;
	T y1;
};

int mandelbrot_plain(uint8_t *buf, int left, int top, int width, int height, int stride, const rect<float> &r, int n,
                     const uint32_t *palette);
