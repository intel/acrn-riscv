/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_PAGE_H__
#define __RISCV_PAGE_H__

#include <asm/pgtable.h>

#define PADDR_BITS              48
#define PADDR_MASK              ((1ULL << PADDR_BITS)-1)
#define PAGE_OFFSET(ptr)        ((vaddr_t)(ptr) & ~PAGE_MASK)
#define PAGE_ALIGN(x) (((x) + PAGE_SIZE - 1) & PAGE_MASK)

#define VADDR_BITS              48
#define VADDR_MASK              0xFFFFFFFFFFFF

#ifndef __ASSEMBLY__

#include <types.h>

#define copy_page(dp, sp) memcpy(dp, sp, PAGE_SIZE)
#define clear_page(page) memset((void *)(page), 0, PAGE_SIZE)

extern void init_s2pt_mem_ops(struct memory_ops *mem_ops, uint16_t vm_id);

#define PAGE_SIZE_GRAN(gran)        (1UL << PAGE_SHIFT_##gran)
#define PAGE_MASK_GRAN(gran)        (-PAGE_SIZE_GRAN(gran))
#define PAGE_ALIGN_GRAN(gran, addr) ((addr + ~PAGE_MASK_##gran) & PAGE_MASK_##gran)

#define PAGE_SHIFT_4K               12
#define PAGE_SIZE_4K                PAGE_SIZE_GRAN(4K)
#define PAGE_MASK_4K                PAGE_MASK_GRAN(4K)
#define PAGE_ALIGN_4K(addr)         PAGE_ALIGN_GRAN(4K, addr)

#define PAGE_SHIFT_16K              14
#define PAGE_SIZE_16K               PAGE_SIZE_GRAN(16K)
#define PAGE_MASK_16K               PAGE_MASK_GRAN(16K)
#define PAGE_ALIGN_16K(addr)        PAGE_ALIGN_GRAN(16K, addr)

#define PAGE_SHIFT_64K              16
#define PAGE_SIZE_64K               PAGE_SIZE_GRAN(64K)
#define PAGE_MASK_64K               PAGE_MASK_GRAN(64K)
#define PAGE_ALIGN_64K(addr)        PAGE_ALIGN_GRAN(64K, addr)

#endif /* !__ASSEMBLY__ */

#endif /* __RISCV_PAGE_H__ */
