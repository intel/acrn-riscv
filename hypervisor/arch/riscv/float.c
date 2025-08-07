/*
 * Copyright (C) 2025 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>

void init_float(void)
{
	uint64_t m = 0x2000;

	asm volatile (
		"csrs sstatus, %0\n\t"
		::"r"(m):
	);
}
