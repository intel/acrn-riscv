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

#define IS_ALIGNED(val, align)	(((val) & ((align) - 1)) == 0)

#ifdef __ASSEMBLY__
#define _AC(X,Y)	X
#define _AT(T,X)	X
#else
#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define _AT(T,X)	((T)(X))
#endif

#ifdef CONFIG_MACRN
#define ACRN_VIRT_START		_AT(uint64_t, 0x80000000)
#define ACRN_MSTACK_TOP		_AT(uint64_t, ACRN_VIRT_START + 0x800000)
#define ACRN_STACK_TOP		_AT(uint64_t, ACRN_VIRT_START + 0x70000)
#define ACRN_MSTACK_SIZE	CONFIG_MSTACK_SIZE
#else
#define ACRN_VIRT_START		_AT(uint64_t, 0x81000000)
#define ACRN_STACK_TOP		_AT(uint64_t, ACRN_VIRT_START + 0x800000)
#endif
#define ACRN_STACK_SIZE		CONFIG_STACK_SIZE
#define ACRN_VSTACK_TOP		_AT(uint64_t, ACRN_VIRT_START + 0x60000)
#define ACRN_VSTACK_SIZE	CONFIG_STACK_SIZE
#define FIXMAP_ADDR(n)		(_AT(uint64_t, ACRN_VIRT_START+ CONFIG_TEXT_SIZE) + (n) * PAGE_SIZE)

#define STACK_ORDER		3
#define STACK_SIZE		(PAGE_SIZE << STACK_ORDER)

#ifndef __ASSEMBLY__

#include <asm/types.h>
#include <asm/init.h>

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
	if (!is_hv(hpa - phys_offset))
		return (void *)hpa;
	else
		return (void *)(hpa - phys_offset);
}

static inline uint64_t hva2hpa(const void *va)
{
	if (!is_hv(va))
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
extern void setup_mem(uint64_t boot_phys_offset);

#endif /* __ASSEMBLY__ */

#endif /* __RISCV_MEM_H__ */
