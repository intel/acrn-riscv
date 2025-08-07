/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <asm/types.h>
#include <asm/init.h>
#include <asm/mem.h>
#include <asm/per_cpu.h>

struct per_cpu_region per_cpu_data[MAX_PCPU_NUM] __aligned(PAGE_SIZE);

void init_percpu_areas(void)
{
	memset(per_cpu_data, 0, sizeof(struct per_cpu_region) * MAX_PCPU_NUM);
}
