/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_CACHE_H__
#define __RISCV_CACHE_H__

/* L1 cache line size */
#define L1_CACHE_SHIFT  (CONFIG_RISCV_L1_CACHE_SHIFT)
#define L1_CACHE_BYTES  (1 << L1_CACHE_SHIFT)

#ifndef L1_CACHE_ALIGN
#define L1_CACHE_ALIGN(x) (((x)+(L1_CACHE_BYTES-1))&~(L1_CACHE_BYTES-1))
#endif

#ifndef SMP_CACHE_BYTES
#define SMP_CACHE_BYTES L1_CACHE_BYTES
#endif

#ifndef __cacheline_aligned
#define __cacheline_aligned __attribute__((__aligned__(SMP_CACHE_BYTES)))
#endif

#define __read_mostly __section(".data.read_mostly")

#ifndef __ASSEMBLY__

#include <types.h>
#include <asm/system.h>

#define __invalidate_dcache_one(R) "fence;"
#define __clean_dcache_one(R) "fence;"
#define __clean_and_invalidate_dcache_one(R) "fence;"

static inline void invalidate_icache(void)
{
	asm volatile ("fence.i");
	dsb();               /* Ensure completion of the flush I-cache */
	isb();
}

static inline void invalidate_icache_local(void)
{
	asm volatile ("fence.i");
	dsb();
	isb();
}

#define MIN_CACHELINE_BYTES 16
extern size_t dcache_line_bytes;
static inline size_t read_dcache_line_bytes(void)
{
	return 128;
}

static inline int invalidate_dcache_va_range(const void *p, unsigned long size)
{
	const void *end = p + size;
	size_t cacheline_mask = dcache_line_bytes - 1;

	dsb();		   /* So the CPU issues all writes to the range */

	if ( (uint64_t)p & cacheline_mask )
	{
		p = (void *)((uint64_t)p & ~cacheline_mask);
		asm volatile (__clean_and_invalidate_dcache_one(0) : : "r" (p));
		p += dcache_line_bytes;
	}
	if ( (uint64_t)end & cacheline_mask )
	{
		end = (void *)((uint64_t)end & ~cacheline_mask);
		asm volatile (__clean_and_invalidate_dcache_one(0) : : "r" (end));
	}

	for ( ; p < end; p += dcache_line_bytes )
		asm volatile (__invalidate_dcache_one(0) : : "r" (p));

	dsb();		   /* So we know the flushes happen before continuing */

	return 0;
}

static inline int clean_dcache_va_range(const void *p, unsigned long size)
{
	const void *end = p + size;
	dsb();		   /* So the CPU issues all writes to the range */
	p = (void *)((uint64_t)p & ~(dcache_line_bytes - 1));
	for ( ; p < end; p += dcache_line_bytes )
		asm volatile (__clean_dcache_one(0) : : "r" (p));
	dsb();		   /* So we know the flushes happen before continuing */
	/* RISCV callers assume that dcache_* functions cannot fail. */
	return 0;
}

static inline int clean_and_invalidate_dcache_va_range
	(const void *p, unsigned long size)
{
	const void *end = p + size;
	dsb();		 /* So the CPU issues all writes to the range */
	p = (void *)((uint64_t)p & ~(dcache_line_bytes - 1));
	for ( ; p < end; p += dcache_line_bytes )
		asm volatile (__clean_and_invalidate_dcache_one(0) : : "r" (p));
	dsb();		 /* So we know the flushes happen before continuing */
	/* RISCV callers assume that dcache_* functions cannot fail. */
	return 0;
}

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
#define alignof __alignof__
#endif

/* Macros for flushing a single small item.  The predicate is always
 * compile-time constant so this will compile down to 3 instructions in
 * the common case. */
#define clean_dcache(x) do {							\
	typeof(x) *_p = &(x);							\
	if ( sizeof(x) > MIN_CACHELINE_BYTES || sizeof(x) > alignof(x) )	\
		clean_dcache_va_range(_p, sizeof(x));				\
	else									\
		asm volatile (							\
			"fence;"   /* Finish all earlier writes */		\
			__clean_dcache_one(0)					\
			"fence;"   /* Finish flush before continuing */		\
			: : "r" (_p), "m" (*_p));				\
} while (0)

#define clean_and_invalidate_dcache(x) do {					\
	typeof(x) *_p = &(x);							\
	if ( sizeof(x) > MIN_CACHELINE_BYTES || sizeof(x) > alignof(x) )	\
		clean_and_invalidate_dcache_va_range(_p, sizeof(x));		\
	else									\
		asm volatile (							\
			"dsb ;"   /* Finish all earlier writes */		\
			__clean_and_invalidate_dcache_one(0)			\
			"dsb ;"   /* Finish flush before continuing */		\
			: : "r" (_p), "m" (*_p));				\
} while (0)

/* Flush the dcache for an entire page. */
extern void flush_page_to_ram(unsigned long mfn, bool sync_icache);
#endif

#endif /* __RISCV_CACHE_H__ */
