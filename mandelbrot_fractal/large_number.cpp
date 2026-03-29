#include "large_number.h"

typedef LargeNumber<4, 4> Large;

void test_me()
{
	Large a{ 0.1 }, b{ 0.2 }, c{ -0.3 };
	a += b;
}