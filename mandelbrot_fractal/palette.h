#pragma once

inline uint32_t pack(double r, double g, double b)
{
	return (static_cast<uint32_t>(r * 255) & 0xFF) | ((static_cast<uint32_t>(g * 255) & 0xFF) << 8) |
		((static_cast<uint32_t>(b * 255) & 0xFF) << 16);
}

inline uint32_t hsv2rgb(double h, double s, double v)
{
	double hh, p, q, t, ff;
	long i;
	double r, g, b;

	if (s <= 0.0) { // < is bogus, just shuts up warnings
		r = v;
		g = v;
		b = v;
		return pack(r, g, b);
	}
	hh = h - ((int)h / 360 * 360);
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = v * (1.0 - s);
	q = v * (1.0 - (s * ff));
	t = v * (1.0 - (s * (1.0 - ff)));

	switch (i) {
	case 0:
		r = v;
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = v;
		b = p;
		break;
	case 2:
		r = p;
		g = v;
		b = t;
		break;

	case 3:
		r = p;
		g = q;
		b = v;
		break;
	case 4:
		r = t;
		g = p;
		b = v;
		break;
	case 5:
	default:
		r = v;
		g = p;
		b = q;
		break;
	}
	return pack(r, g, b);
}

constexpr size_t palette_size = 1024;
struct Palette
{
	Palette()
	{
		for (int i = 0; i < palette_size; ++i) {
			hi[i] = hsv2rgb(double(i) * 360 * 8 / palette_size, 0.8, 0.8);
			lo[i] = hsv2rgb(double(i) * 360 * 8 / palette_size, 0.8, 0.4);
		}
	}

	uint32_t hi[palette_size];
	uint32_t lo[palette_size];
};

