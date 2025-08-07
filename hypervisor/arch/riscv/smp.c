/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <asm/smp.h>
#include <asm/sbi.h>
#include <asm/per_cpu.h>
#include <schedule.h>

struct smp_ops *smp_ops = NULL;

void register_smp_ops(struct smp_ops *ops)
{
	if (!smp_ops)
		smp_ops = ops;
}

void cpu_do_idle(void)
{
	asm volatile ("wfi"::);
}

int g_cpus = 1;

void cpu_dead(void)
{
	while(1);
}

int get_pcpu_nums(void)
{
	return g_cpus;
}

int smp_platform_init(void)
{
#ifdef CONFIG_MACRN
	init_clint_ipi();
#else
	init_sbi_ipi();
#endif
	smp_init_cpus();

	return 0;
}
