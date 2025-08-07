/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_MEM_H__
#define __RISCV_MEM_H__

#include <asm/config.h>

typedef uint64_t mfn_t;
typedef uint64_t paddr_t;

#define pfn_to_paddr(pfn) ((paddr_t)(pfn) << PAGE_SHIFT)
#define paddr_to_pfn(pa)  ((uint64_t)((pa) >> PAGE_SHIFT))
#define mfn_to_maddr(mfn)   pfn_to_paddr(mfn)
#define maddr_to_mfn(ma)    paddr_to_pfn(ma)
#define gfn_to_gaddr(gfn)   pfn_to_paddr(gfn)

extern paddr_t phys_offset;
static inline void *hpa2hva(uint64_t hpa)
{
	if ( !is_kernel(hpa - phys_offset) )
		return (void *)hpa;
	else
		return (void *)(hpa - phys_offset);
}

static inline uint64_t hva2hpa(const void *va)
{
	if ( !is_kernel(va) )
		return (uint64_t)va;
	else
		return (uint64_t)va + phys_offset;
}

#ifndef CONFIG_MACRN
extern int init_secondary_pagetables(int cpu);
static inline void switch_satp(uint64_t satp)
{
       asm volatile (
		"csrw satp, %0\n\t" \
		"sfence.vma"
		:: "r"(satp)
		: "memory"
       );
}
#endif
extern void setup_mem(unsigned long boot_phys_offset);

#endif /* __RISCV_MEM_H__ */
