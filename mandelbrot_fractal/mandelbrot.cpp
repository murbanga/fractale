#include <stdint.h>
#include "mandelbrot.h"

template<>
int mandelbrot<float>(Image &image, int left, int top, int width, int height, const Rect<float> &r, int n,
                     const uint32_t *palette) {
	constexpr float max_radius = 2 * 2;
	const float scalex = (r.x1 - r.x0) / image.width;
	const float scaley = (r.y1 - r.y0) / image.height;
	uint32_t *pixels = reinterpret_cast<uint32_t *>(image.buf);

	for (int y = top; y < height; ++y) {
		for (int x = left; x < width; ++x) {
			const float u0 = float(x) * scalex + r.x0;
			const float v0 = float(y) * scaley + r.y0;
			float u = 0, v = 0;

			int i = 0;
			while (u * u + v * v < max_radius && i < n) {
				float nextu = u * u - v * v + u0;
				v = 2 * u * v + v0;
				u = nextu;
				i++;
			}

			int idx = x + y * image.width;
			if (i == n)
				pixels[idx] = 0;
			else
				pixels[idx] = palette[i];
		}
	}
	return 0;
}