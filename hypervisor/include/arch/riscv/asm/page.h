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

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1 << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE-1))
#define PAGE_FLAG_MASK		(~0)

#define clear_page(page)	( {memset((void *)(page), 0, PAGE_SIZE);} )

#endif /* __RISCV_PAGE_H__ */
