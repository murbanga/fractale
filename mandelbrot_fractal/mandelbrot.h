#pragma once

template <typename T> struct rect {
	T x0;
	T x1;
	T y0;
	T y1;
};

struct Image {
	uint8_t *buf = nullptr;
	size_t buf_size = 0;
	int width = 1280;
	int height = 720;
	int idx = 0;
};

int mandelbrot(Image &image, int left, int top, int width, int height, const rect<float> &r, int n,
                     const uint32_t *palette);
