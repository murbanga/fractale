#pragma once
#include <variant>
#include <string>
#include <assert.h>

#define LARGE_NUMBERS 0

#if LARGE_NUMBERS
#define BOOST_MP_USE_QUAD
// #include <boost/multiprecision/float128.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

using float128 = boost::multiprecision::number<boost::multiprecision::cpp_bin_float<96>>;
using fp = std::variant<float, double, float128>;
#else
using fp = std::variant<float, double>;
#endif

#include "palette.h"

template <typename T> struct Rect {
	T x0;
	T x1;
	T y0;
	T y1;

	Rect<T> normalize() const
	{
		using namespace std;
		return {min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1)};
	}

	bool valid() const { return x0 >= 0 && y0 >= 0; /* && x1 >= 0 && y1 >= 0; */ }
};

struct Image {
	uint8_t *buf = nullptr;
	size_t buf_size = 0;
	int width = 0;
	int height = 0;
	int idx = 0;
};

template <typename T> Rect<T> collapse(const Rect<fp> &fractal)
{
	using namespace std;
	return {get<T>(fractal.x0), get<T>(fractal.x1), get<T>(fractal.y0), get<T>(fractal.y1)};
}

template <typename A, typename B> A e(B b) { return static_cast<A>(b); }

#if LARGE_NUMBERS
template <typename A> A e(const float128 &b) { return b.convert_to<A>(); }
#endif

inline std::string fptostr(const fp &a)
{
	char buf[256];
	switch (a.index()) {
	case 0: {
		float x = std::get<0>(a);
		snprintf(buf, sizeof(buf), "%.16f", x);
		return buf;
	}
	case 1: {
		double x = std::get<1>(a);
		snprintf(buf, sizeof(buf), "%.16f", x);
		return buf;
	}
#if LARGE_NUMBERS
	case 2: {
		float128 x = std::get<2>(a);
		return x.str();
	}
#endif
	}
	assert(false);
	return "";
}

template <typename T>
int mandelbrot(Image &image, int left, int top, int width, int height, const Rect<T> &r, int n, const Palette &palette);
