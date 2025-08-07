/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_LIB_BITOPS_H__
#define __RISCV_LIB_BITOPS_H__

#include <types.h>
#include <asm/config.h>
/*
 * Non-atomic bit manipulation.
 *
 * Implemented using atomics to be interrupt safe. Could alternatively
 * implement with local interrupt masking.
 */
#define __set_bit(n,p)		set_bit(n,p)
#define __clear_bit(n,p)	clear_bit(n,p)

#define BITOP_BITS_PER_WORD	32
#define BITOP_MASK(nr)		(1UL << ((nr) % BITOP_BITS_PER_WORD))
#define BITOP_WORD(nr)		((nr) / BITOP_BITS_PER_WORD)
#define BITS_PER_BYTE		8

#define ADDR (*(volatile int *) addr)
#define CONST_ADDR (*(const volatile int *) addr)

#define always_inline __inline__ __attribute__ ((__always_inline__))
#ifdef CONFIG_MACRN
static always_inline unsigned long ffsl(unsigned long x)
{
	int m = 0x1, i;

	for (i = 0; i < BITS_PER_LONG; i++) {
		if ((x & m) != 0)
			break;
		m <<= 1;
	};

	return i;
}

static inline int flsl(unsigned long x)
{
	unsigned long m = 0x8000000000000000, i;

	for (i = 1; i < BITS_PER_LONG; i++) {
		if ((x & m) != 0)
			break;
		m >>= 1;
	};

	return BITS_PER_LONG - i;
}
#else
static always_inline unsigned long ffsl(unsigned long x)
{
	asm volatile ("ctz %0, %1" : "=r"(x) : "r"(x));
	return x;
}

static inline int flsl(unsigned long x)
{
	asm volatile ("clz %0, %1" : "=r"(x) : "r"(x));
	return BITS_PER_LONG - 1 - x;
}
#endif

#define ffz(x)  ffsl(~(x))

/*
 * Atomic bitops
 *
 * The helpers below *should* only be used on memory shared between
 * trusted threads or we know the memory cannot be accessed by another
 * thread.
 */

extern void set_bit(int nr, volatile void *p);
extern void clear_bit(int nr, volatile void *p);
extern bool test_bit(int nr, uint64_t bits);
extern int test_and_set_bit(int nr, volatile void *p);
extern int test_and_clear_bit(int nr, volatile void *p);
extern int test_and_change_bit(int nr, volatile void *p);
extern void clear_mask16(uint16_t mask, volatile void *p);

/**
 * __test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static inline int __test_and_set_bit(int nr, volatile void *addr)
{
	unsigned int mask = BITOP_MASK(nr);
	volatile unsigned int *p = ((volatile unsigned int *)addr) + BITOP_WORD(nr);
	unsigned int old = *p;

	*p = old | mask;
	return (old & mask) != 0;
}

/**
 * __test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.
 * If two examples of this operation race, one can appear to succeed
 * but actually fail.  You must protect multiple accesses with a lock.
 */
static inline int __test_and_clear_bit(int nr, volatile void *addr)
{
	unsigned int mask = BITOP_MASK(nr);
	volatile unsigned int *p = ((volatile unsigned int *)addr) + BITOP_WORD(nr);
	unsigned int old = *p;

	*p = old & ~mask;
	return (old & mask) != 0;
}

/* WARNING: non atomic and it can be reordered! */
static inline int __test_and_change_bit(int nr, volatile void *addr)
{
	unsigned int mask = BITOP_MASK(nr);
	volatile unsigned int *p = ((volatile unsigned int *)addr) + BITOP_WORD(nr);
	unsigned int old = *p;

	*p = old ^ mask;
	return (old & mask) != 0;
}

static inline int ffs(unsigned int x)
{
	asm volatile ("ctzw %0, %1" : "=r" (x) : "r" (x));
	return x + 1;
}

static inline int fls(unsigned int x)
{
	asm volatile ("clzw %0, %1" : "=r" (x) : "r" (x));
	return 32 - x;
}

/**
 * find_first_set_bit - find the first set bit in @word
 * @word: the word to search
 *
 * Returns the bit-number of the first set bit (first bit being 0).
 * The input must *not* be zero.
 */
static inline unsigned int find_first_set_bit(unsigned long word)
{
	return ffsl(word) - 1;
}

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */

static inline unsigned int generic_hweight32(unsigned int w)
{
	w -= (w >> 1) & 0x55555555;
	w =  (w & 0x33333333) + ((w >> 2) & 0x33333333);
	w =  (w + (w >> 4)) & 0x0f0f0f0f;

	#ifdef CONFIG_HAS_FAST_MULTIPLY
		return (w * 0x01010101) >> 24;
	#endif

	w += w >> 8;

	return (w + (w >> 16)) & 0xff;
}

static inline unsigned int generic_hweight16(unsigned int w)
{
	w -= ((w >> 1) & 0x5555);
	w =  (w & 0x3333) + ((w >> 2) & 0x3333);
	w =  (w + (w >> 4)) & 0x0f0f;

	return (w + (w >> 8)) & 0xff;
}

static inline unsigned int generic_hweight8(unsigned int w)
{
	w -= ((w >> 1) & 0x55);
	w =  (w & 0x33) + ((w >> 2) & 0x33);

	return (w + (w >> 4)) & 0x0f;
}

static inline unsigned int generic_hweight64(uint64_t w)
{
	if ( BITS_PER_LONG < 64 )
		return generic_hweight32(w >> 32) + generic_hweight32(w);

	w -= (w >> 1) & 0x5555555555555555ul;
	w =  (w & 0x3333333333333333ul) + ((w >> 2) & 0x3333333333333333ul);
	w =  (w + (w >> 4)) & 0x0f0f0f0f0f0f0f0ful;

	#ifdef CONFIG_HAS_FAST_MULTIPLY
		return (w * 0x0101010101010101ul) >> 56;
	#endif

	w += w >> 8;
	w += w >> 16;

	return (w + (w >> 32)) & 0xFF;
}

static inline unsigned long hweight_long(unsigned long w)
{
	return sizeof(w) == 4 ? generic_hweight32(w) : generic_hweight64(w);
}

/**
 * hweightN - returns the hamming weight of a N-bit word
 * @x: the word to weigh
 *
 * The Hamming Weight of a number is the total number of bits set in it.
 */
#define hweight64(x) generic_hweight64(x)
#define hweight32(x) generic_hweight32(x)
#define hweight16(x) generic_hweight16(x)
#define hweight8(x) generic_hweight8(x)

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
