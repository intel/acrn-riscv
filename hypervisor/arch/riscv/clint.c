/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <asm/lib/bits.h>
#include <asm/cpu.h>
#include <asm/smp.h>
#include <asm/per_cpu.h>

static int do_swi(int cpu)
{
	int val = 0x1;
	uint64_t off = CLINT_SWI_REG;

	off += (uint64_t)cpu * 4;
	asm volatile (
		"sw %0, 0(%1)"
		:: "r"(val), "r"(off)
		:"memory"
	);
	dsb();

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

static struct smp_ops clint_smp_ops =
	{do_swi, send_single_swi, send_dest_ipi_mask, ipi_start_cpu};

/**
 * @pre pcpu_id < 8U
 */
void init_clint_ipi(void)
{
	register_smp_ops(&clint_smp_ops);
}
