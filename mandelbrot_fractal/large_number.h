#pragma once
#include <intrin.h>
#include <stdint.h>

template <int N, int POINT_BIT>
class LargeNumber
{
public:
	LargeNumber()
	{
		memset(mantissa, 0, sizeof(mantissa));
	}

	explicit LargeNumber(float a)
	{
		int ai = floor(a);
		float af = a - ai;
	}

	explicit LargeNumber(double a)
	{

	}

	LargeNumber<N, POINT_BIT>& operator += (const LargeNumber<N, POINT_BIT> &a)
	{
		uint8_t c = 0;
		for (int i = N - 1; i >= 0; --i)
		{
			c = _addcarry_u64(c, mantissa[i], a.mantissa[i], &mantissa[i]);
		}
		//assert(c == 0);
		return *this;
	}

	LargeNumber<N, POINT_BIT>& operator -= (const LargeNumber<N, POINT_BIT> &a)
	{
		uint8_t c = 0;
		for (int i = N - 1; i >= 0; --i)
		{
			c = _subborrow_u64(c, mantissa[i], a.mantissa[i], &mantissa[i]);
		}
		assert(c == 0);
		return *this;
	}

	LargeNumber<N, POINT_BIT>& operator *= (const LargeNumber<N, POINT_BIT> &a)
	{

	}

private:
	uint64_t mantissa[N];
};
