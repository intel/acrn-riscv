/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <asm/lib/bits.h>
#include <asm/cpu.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <asm/per_cpu.h>

static int do_swi(int cpu)
{
	int val = 0x1;
	uint64_t off = CLINT_SWI_REG;

	off += (uint64_t)cpu * 4;
	mmio_writel(val, (void *)off);

	return 0;
}

static void send_single_swi(uint16_t pcpu_id, uint64_t vector)
{
	set_bit(vector, &per_cpu(swi_vector, pcpu_id).type);
	do_swi(pcpu_id);
}

static void send_dest_ipi_mask(uint64_t dest_mask, uint64_t vector)
{
	uint16_t pcpu_id;
	uint64_t mask = dest_mask;

	pcpu_id = ffs64(mask);
	if (pcpu_id--)
		return;
	while (pcpu_id < NR_CPUS) {
		clear_bit(pcpu_id, &mask);
		send_single_swi(pcpu_id, vector);
		pcpu_id = ffs64(mask);
	}
}

static int ipi_start_cpu(int cpu, __unused uint64_t addr, __unused uint64_t arg)
{
	return do_swi(cpu);
}

static inline void send_rfence_mask(uint64_t dest_mask, uint64_t addr, uint64_t size)
{}

static inline void send_hfence_mask(uint64_t dest_mask, uint64_t addr, uint64_t size)
{}

static struct smp_ops clint_smp_ops =
	{do_swi, send_single_swi, send_dest_ipi_mask, ipi_start_cpu, send_rfence_mask, send_hfence_mask};

static void clint_preinit_timer(void)
{
	for (int i = BSP_CPU_ID; i < NR_CPUS; i++)
		mmio_writeq(CLINT_DISABLE_TIMER, (void *)CLINT_MTIMECMP(i));
}

static uint64_t clint_get_tick(void)
{
	return mmio_readq((void *)CLINT_MTIME);
}

static int clint_set_deadline(uint64_t deadline)
{
	uint16_t cpu = get_pcpu_id();

	mmio_writeq(deadline, (void *)CLINT_MTIMECMP(cpu));

	return 0;
}

static struct timer_ops clint_timer_ops =
	{clint_preinit_timer, clint_get_tick, clint_set_deadline};

void init_clint_ipi(void)
{
	register_smp_ops(&clint_smp_ops);
}

void init_clint_timer(void)
{
	register_timer_ops(&clint_timer_ops);
}
