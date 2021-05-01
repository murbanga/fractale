#include <stdint.h>

int mandelbrot_plain(uint8_t *buf, int width, int height, float x0, float y0, float x1, float y1, int n, const uint32_t *palette)
{
	constexpr float max_radius = 2 * 2;
	const float scalex = (x1 - x0) / width;
	const float scaley = (y1 - y0) / height;
	uint32_t *pixels = reinterpret_cast<uint32_t *>(buf);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const float u0 = float(x) * scalex + x0;
			const float v0 = float(y) * scaley + y0;
			float u = 0, v = 0;

			int i = 0;
			while (u * u + v * v < max_radius && i < n)
			{
				float nextu = u * u - v * v + u0;
				v = 2 * u * v + v0;
				u = nextu;
				i++;
			}

			int idx = x + y * width;
			pixels[idx] = palette[i];
		}
	}
	return 0;
}