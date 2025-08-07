/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <asm/lib/bits.h>
#include <asm/system.h>

void set_bit(int nr, volatile void *p)
{
	*(uint64_t *)p |= (1 << nr);
}

void clear_bit(int nr, volatile void *p)
{
	*(uint64_t *)p &= ~(1 << nr);
}

bool test_bit(int nr, uint64_t bits)
{
	return !!(bits & (1 << nr));
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

uint32_t bit_weight(uint64_t bits)
{
	return __builtin_popcountl(bits);
}
