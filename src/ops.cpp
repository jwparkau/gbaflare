#include "ops.h"

u32 ror(u32 x, u32 n, bool &carry)
{
	if (n == 0) {
		return x;
	}

	n = n & BITMASK(5);
	if (n == 0) {
		carry = x & BIT(31);
		return x;
	}

	carry = x & BIT(n - 1);
	return (x >> n) | (x << (32 - n));
}

u32 rrx(u32 x, bool &carry)
{
	carry = x & BIT(0);
	return (carry << 31) | (x >> 1);
}

u32 asr(u32 x, u32 n, bool &carry)
{
	if (n == 0) {
		return x;
	}

	if (n >= 32) {
		carry = x & BIT(31);
		return carry ? 0xFFFF'FFFF : 0;
	}

	carry = x & BIT(n - 1);
	return (s32)x >> n;
}

u32 lsr(u32 x, u32 n, bool &carry)
{
	if (n == 0) {
		return x;
	}

	if (n == 32) {
		carry = x & BIT(31);
		return 0;
	}

	if (n > 32) {
		carry = 0;
		return 0;
	}

	carry = x & BIT(n - 1);
	return x >> n;
}

u32 lsl(u32 x, u32 n, bool &carry)
{
	if (n == 0) {
		return x;
	}

	if (n == 32) {
		carry = x & BIT(0);
		return 0;
	}

	if (n > 32) {
		carry = 0;
		return 0;
	}

	carry = x & BIT(32 - n);
	return x << n;
}
