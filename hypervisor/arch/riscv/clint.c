/*
 * Copyright (C) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <acrn/bitops.h>
#include <asm/cpu.h>
#include <acrn/percpu.h>

struct swi_vector {
	uint32_t type;
	uint64_t param;
};

static DEFINE_PER_CPU(struct swi_vector, swi_vector);
/**
 * @pre pcpu_id < 8U
 */
void init_clint(uint16_t pcpu_id)
{
	per_cpu(swi_vector, pcpu_id).type = 0;
	per_cpu(swi_vector, pcpu_id).param = 0;
}

void
send_startup_ipi(uint16_t dest_pcpu_id, uint64_t cpu_startup_start_address)
{
}

void send_dest_ipi_mask(uint32_t dest_mask, uint32_t vector)
{
	uint16_t pcpu_id;
	uint32_t mask = dest_mask;

	pcpu_id = ffs64(mask);
	if (pcpu_id--)
		return;
	while (pcpu_id < NR_CPUS) {
		clear_bit(pcpu_id, &mask);
		send_single_swi(pcpu_id, vector);
		pcpu_id = ffs64(mask);
	}
}

#define CLINT_SWI_REG 0x02000000
void send_single_swi(uint16_t pcpu_id, uint32_t vector)
{
	unsigned long reg = CLINT_SWI_REG + pcpu_id * 4;
	int val = 0x1;

	per_cpu(swi_vector, pcpu_id).type |= vector;
	asm volatile (
		"sw %1, 0(%0) \n\t"
		:: "r" (reg), "r" (val)
	);
}
