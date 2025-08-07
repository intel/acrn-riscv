/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_TLB_H__
#define __RISCV_TLB_H__

#include <asm/system.h>

#define HTLB_HELPER(name)		\
static inline void name(void)		\
{					\
	asm volatile(			\
		"hfence.gvma;"		\
		: : : "memory");	\
}

#define STLB_HELPER(name)		\
static inline void name(void)		\
{					\
	asm volatile(			\
		"sfence.vma;"		\
		: : : "memory");	\
}

#ifdef CONFIG_MACRN
STLB_HELPER(flush_guest_tlb_local);

#else /* !CONFIG_MACRN */

HTLB_HELPER(flush_guest_tlb_local);
STLB_HELPER(flush_acrn_tlb_local);

static inline void  __flush_acrn_tlb_entry(uint64_t va)
{
	asm volatile("sfence.vma;" : : "r" (va>>PAGE_SHIFT) : "memory");
}

static inline void flush_acrn_tlb_range_va(uint64_t va, unsigned long size)
{
	uint64_t end = va + size;

	dsb();
	while ( va < end )
	{
		__flush_acrn_tlb_entry(va);
		va += PAGE_SIZE;
	}
	dsb();
	isb();
}
#endif

static inline void flush_tlb_addr(uint64_t addr)
{
	asm volatile("sfence.vma %0":: "r"(addr): "memory");
}

static inline void flush_tlb_asid(uint64_t asid)
{
	asm volatile("sfence.vma x0, %0":: "r"(asid): "memory");
}

static inline void flush_tlb_addr_asid(uint64_t addr, uint64_t asid)
{
	asm volatile("sfence.vma %0, %1":: "r"(addr), "r"(asid): "memory");
}

#endif /* __RISCV_TLB_H__ */
