#include <stdint.h>
#include "mandelbrot.h"

template <typename T>
int mandelbrot<T>(Image &image, int left, int top, int width, int height, const Rect<T> &r, int n,
                  const uint32_t *palette)
{
	constexpr T max_radius = 2 * 2;
	const T scalex = (r.x1 - r.x0) / image.width;
	const T scaley = (r.y1 - r.y0) / image.height;
	uint32_t *pixels = reinterpret_cast<uint32_t *>(image.buf);

	for (int y = top; y < height; ++y) {
		for (int x = left; x < width; ++x) {
			const T u0 = T(x) * scalex + r.x0;
			const T v0 = T(y) * scaley + r.y0;
			T u = 0, v = 0;

			int i = 0;
			while (u * u + v * v < max_radius && i < n) {
				T nextu = u * u - v * v + u0;
				v = 2 * u * v + v0;
				u = nextu;
				i++;
			}

			int idx = x + y * image.width;
			if (i == n)
				pixels[idx] = 0;
			else
				pixels[idx] = palette[i];

#if 0
			if (x % 20 == 0 || y % 20 == 0)
				pixels[idx] = 0;
			else
				pixels[idx] = palette[20];
#endif
		}
	}
	return 0;
}

template
int mandelbrot<float>(Image &image, int left, int top, int width, int height, const Rect<float> &r, int n,
                      const uint32_t *palette);

template
int mandelbrot<double>(Image &image, int left, int top, int width, int height, const Rect<double> &r, int n,
                       const uint32_t *palette);
