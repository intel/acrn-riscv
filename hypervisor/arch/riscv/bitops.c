#include <acrn/bitops.h>
#include <asm/system.h>

void set_bit(int nr, volatile void *p)
{
	*(uint64_t *)p |= 1 << nr;
}

void clear_bit(int nr, volatile void *p)
{
	*(uint64_t *)p &= ~(1 << nr);
}

int test_and_set_bit(int nr, volatile void *p)
{
	return 0;
}

int test_and_clear_bit(int nr, volatile void *p)
{
	return 0;
}

int test_and_change_bit(int nr, volatile void *p)
{
	return 0;
}
