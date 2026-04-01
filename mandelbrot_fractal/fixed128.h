#pragma once
#include <stdint.h>
#include <intrin.h>

constexpr int inte = 16;
constexpr int frac = 112;
static_assert(inte + frac == 128);

struct Fixed128 {
	uint64_t hi;
	uint64_t lo;

	Fixed128() = default;
	Fixed128(double x);
	Fixed128(uint64_t a, uint64_t b);
	template <typename T> T convert_to() const;
	std::string str() const;
};

inline Fixed128 operator+(const Fixed128 &a, const Fixed128 &b)
{
	int c = 0;
	Fixed128 d;
	c = _addcarry_u64(c, a.lo, b.lo, &d.lo);
	_addcarry_u64(c, a.hi, b.hi, &d.lo);
	return d;
}

inline Fixed128 operator-(const Fixed128 &a, const Fixed128 &b)
{
	int borrow = 0;
	Fixed128 d;
	borrow = _subborrow_u64(borrow, a.lo, b.lo, &d.lo);
	_subborrow_u64(borrow, a.hi, b.hi, &d.hi);
	return d;
}

inline Fixed128 mul128(uint64_t a, uint64_t b) { return {__umulh(a, b), a * b}; }

inline Fixed128 operator*(const Fixed128 &a, const Fixed128 &b)
{
	/*
	 *              a.hi a.lo
	 *            x b.hi b.lo
	 *            -----------
	 *              x.hi x.lo
	 *         y.hi y.lo
	 *         z.hi z.lo
	 *    w.hi w.lo
	 *    -------------------
	 */
	const Fixed128 zero = {0, 0};
	const Fixed128 maxval = {0x7FFF'FFFF'FFFF'FFFFUL, 0xFFFF'FFFF'FFFF'FFFFUL};
	const Fixed128 negmaxval = {0x8000'0000'0000'0000UL, 0x0000'0000'0000'0000UL};

	uint64_t signa = (a.hi & 0x8000'0000'0000'0000UL);
	uint64_t signb = (b.hi & 0x8000'0000'0000'0000UL);

	Fixed128 ma = signa ? zero - a : a;
	Fixed128 mb = signb ? zero - b : b;

	Fixed128 x, y, z, w;
	x = mul128(a.lo, b.lo);
	y = mul128(a.hi, b.lo);
	z = mul128(a.lo, b.hi);
	w = mul128(a.hi, b.hi);

	Fixed128 hi, lo;
	hi.hi = w.hi;
	hi.lo = y.hi + z.hi + w.lo;
	lo.hi = x.hi + y.lo + z.lo;
	lo.lo = x.lo;

	int64_t val = (int64_t)hi.hi >> (64 - inte - inte);
	if (abs(val) > 0x7fff)
		return maxval;
	else {
		Fixed128 res;
		res.hi = (hi.hi << inte) | (hi.lo >> (64 - inte));
		res.lo = (hi.lo << inte) | (lo.hi >> (64 - inte));
		return res;
	}
	/*	if (res.hi & 0x8000'0000'0000'0000UL)
	                return maxval;
	        else
	                return signa ^ signb ? zero - res : res;*/
}

Fixed128 operator /(const Fixed128 &a, double den)
{
	uint64_t inte = static_cast<uint64_t>(den);

}

//Fixed128 operator /(const Fixed128 &a, const Fixed128 &den);

bool operator < (const Fixed128 &a, const Fixed128 &b)
{
	if (a.hi == b.hi)
	{
		return a.lo < b.lo;
	}
	else {
		return (int64_t)a.hi < (int64_t)b.hi;
	}
}

#if 0
https://stackoverflow.com/questions/16365840/128-bit-integers-supporting-and-in-the-intel-c-compiler

GCC and Clang have the __int128_t and __uint128_t extensions for 128 - bit integer arithmetic.

I was hopeful that __m128i would give something similar for the Intel C Compiler, but(if it's even possible) it looks to me like I'd have to write explicit SSE2 function calls in order to use __m128i, instead of using "built-in" operators like + , -, *, / , and%.I was hoping to do something like this (this doesn't work):

#if defined(__INTEL_COMPILER) && defined(__SSE2__)
#include "xmmintrin.h"
        typedef __u128 uint128_t;
#elif defined(__GNUC__)
        typedef __uint128_t uint128_t;
#else
#error For 128-bit arithmetic we need GCC or ICC, or uint128_t
#endif


From what I can tell, at least icc 13.0.1 + support __int128_t and __uint128_t.Courtesy of Matt Godbolt's Compiler Explorer:

__int128_t ai(__int128_t x, __int128_t y)
{
        return x + y;
}

__int128_t mi(__int128_t x, __int128_t y)
{
        return x * y;
}

__int128_t di(__int128_t x, __int128_t y)
{
        return x / y;
}

__int128_t ri(__int128_t x, __int128_t y)
{
        return x % y;
}
compiles to :

L__routine_start_ai_0:
ai:
add       rdi, rdx                                      #2.14
mov       rax, rdi                                      #2.14
adc       rsi, rcx                                      #2.14
mov       rdx, rsi                                      #2.14
ret                                                     #2.14
L__routine_start_mi_1:
mi:
mov       rax, rdi                                      #6.14
imul      rsi, rdx                                      #6.14
imul      rcx, rdi                                      #6.14
mul       rdx                                           #6.14
add       rsi, rcx                                      #6.14
add       rdx, rsi                                      #6.14
ret                                                     #6.14
L__routine_start_di_2:
di:
push      rsi                                           #9.44
call      __divti3                                      #10.14
pop       rcx                                           #10.14
ret                                                     #10.14
L__routine_start_ri_3:
ri:
push      rsi                                           #13.44
call      __modti3                                      #14.14
pop       rcx                                           #14.14
ret
#endif