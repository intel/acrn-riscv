/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_LIB_BITOPS_H__
#define __RISCV_LIB_BITOPS_H__

#include <asm/types.h>
#include <asm/cpu.h>

#define __set_bit(n,p)		set_bit(n,p)
#define __clear_bit(n,p)	clear_bit(n,p)

#define always_inline __inline__ __attribute__ ((__always_inline__))
#ifdef CONFIG_MACRN
static always_inline uint64_t ffsl(uint64_t x)
{
	int m = 0x1, i;

	for (i = 0; i < BITS_PER_LONG; i++) {
		if ((x & m) != 0)
			break;
		m <<= 1;
	};

	return i;
}

static inline int flsl(uint64_t x)
{
	uint64_t m = 0x8000000000000000, i;

	for (i = 1; i < BITS_PER_LONG; i++) {
		if ((x & m) != 0)
			break;
		m >>= 1;
	};

	return BITS_PER_LONG - i;
}
#else
static always_inline uint64_t ffsl(uint64_t x)
{
	asm volatile ("ctz %0, %1" : "=r"(x) : "r"(x));
	return x;
}

static inline int flsl(uint64_t x)
{
	asm volatile ("clz %0, %1" : "=r"(x) : "r"(x));
	return BITS_PER_LONG - 1 - x;
}
#endif

#define ffz(x)  ffsl(~(x))

extern void set_bit(int nr, volatile void *p);
extern void clear_bit(int nr, volatile void *p);
extern bool test_bit(int nr, uint64_t bits);
extern int test_and_set_bit(int nr, volatile void *p);
extern int test_and_clear_bit(int nr, volatile void *p);
extern int test_and_change_bit(int nr, volatile void *p);
extern void clear_mask16(uint16_t mask, volatile void *p);

static inline int ffs(uint32_t x)
{
	asm volatile ("ctzw %0, %1" : "=r" (x) : "r" (x));
	return x + 1;
}

static inline int fls(uint32_t x)
{
	asm volatile ("clzw %0, %1" : "=r" (x) : "r" (x));
	return 32 - x;
}

static inline uint32_t find_first_set_bit(uint64_t word)
{
	return ffsl(word) - 1;
}

static inline void bitmap_set_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	*addr |= 1 << (uint64_t)nr_arg;
}

static inline void bitmap_clear_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	*addr &= ~(1 << (uint64_t)nr_arg);
}

static inline void bitmap_set_nolock(uint16_t nr_arg, volatile uint64_t *addr)
{}

static inline bool bitmap_clear_nolock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap_test_and_set_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap_test_and_clear_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	if (!!(*addr & (1 << nr_arg))) {
		*addr &= ~(1 << nr_arg);
		return true;
	} else {
		return false;
	}
}

static inline bool bitmap_test(uint16_t nr, const volatile uint64_t *addr)
{
	return !!(*addr & (1 << nr));
}

static inline bool bitmap32_set_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_clear_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_set_nolock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_clear_nolock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_test_and_set_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_test_and_clear_lock(uint16_t nr_arg, volatile uint64_t *addr)
{
	return true;
}

static inline bool bitmap32_test(uint16_t nr, const volatile uint32_t *addr)
{
	return true;
}

uint32_t bit_weight(uint64_t bits);

#define ffs64 ffsl
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)

/*bit scan forward for the least significant bit '0'*/
static inline uint16_t ffz64(uint64_t value)
{
	return ffs64(~value);
}

/*
 * find the first zero bit in a uint64_t array.
 * @pre: the size must be multiple of 64.
 */
static inline uint64_t ffz64_ex(const uint64_t *addr, uint64_t size)
{
	uint64_t ret = size;
	uint64_t idx;

	for (idx = 0UL; (idx << 6U) < size; idx++) {
		if (addr[idx] != ~0UL) {
			ret = (idx << 6U) + ffz64(addr[idx]);
			break;
		}
	}

	return ret;
}

#endif /* __RISCV_LIB_BITOPS_H__ */
