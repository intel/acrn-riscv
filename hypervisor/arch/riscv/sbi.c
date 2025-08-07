/*
 * Copyright (C) 2025 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */
#include <lib/types.h>
#include <asm/lib/bits.h>
#include <asm/cpu.h>
#include <asm/per_cpu.h>
#include <asm/smp.h>
#include <asm/sbi.h>

static sbi_ret sbi_ecall(uint64_t arg0, uint64_t arg1, uint64_t arg2,
				uint64_t arg3, uint64_t arg4, uint64_t arg5,
				uint64_t func, uint64_t ext)
{
	sbi_ret ret;

	register uint64_t a0 asm ("a0") = arg0;
	register uint64_t a1 asm ("a1") = arg1;
	register uint64_t a2 asm ("a2") = arg2;
	register uint64_t a3 asm ("a3") = arg3;
	register uint64_t a4 asm ("a4") = arg4;
	register uint64_t a5 asm ("a5") = arg5;
	register uint64_t a6 asm ("a6") = func;
	register uint64_t a7 asm ("a7") = ext;

	asm volatile (
		"ecall \n\t"
		:"+r" (a0), "+r" (a1)
		:"r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		: "memory"
	);

	ret.error = a0;
	ret.value = a1;

	return ret;
}

static int do_swi(int cpu)
{
	uint64_t mask = 1UL << cpu;
	sbi_ret ret;

	ret = sbi_ecall(mask, 0, 0, 0, 0, 0, SBI_TYPE_IPI_SEND_IPI, SBI_ID_IPI);
	if (ret.error != SBI_SUCCESS)
		pr_err("%s: %lx", __func__, ret.error);

	return ret.error;
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

static int ipi_start_cpu(int cpu, uint64_t addr, uint64_t arg)
{
	sbi_ret ret;

	ret = sbi_ecall((uint64_t)cpu, addr, arg, 0, 0, 0, SBI_TYPE_HSM_HART_START,
			SBI_ID_HSM);
	if (ret.error != SBI_SUCCESS)
		pr_err("%s: %lx", __func__, ret.error);

	return ret.error;
}

static struct smp_ops sbi_smp_ops =
	{do_swi, send_single_swi, send_dest_ipi_mask, ipi_start_cpu};

void init_sbi_ipi(void)
{
	register_smp_ops(&sbi_smp_ops);
}

static void sbi_preinit_timer(void)
{
	return;
}

static int sbi_set_deadline(uint64_t deadline)
{
	sbi_ret ret = {0, 0};

#ifdef CONFIG_SSTC
	asm volatile ("csrw stimecmp, %0"::"r"(deadline):);
#else
	ret = sbi_ecall(deadline, 0, 0, 0, 0, 0, SBI_TYPE_TIME_SET_TIMER,
			SBI_ID_TIMER);
#endif
	return ret.error;
}

static uint64_t sbi_get_tick(void)
{
	uint64_t tick;
	asm volatile (
		"rdtime %0":"=r"(tick):: "memory");
	return tick;
}
static struct timer_ops sbi_timer_ops =
	{sbi_preinit_timer, sbi_get_tick, sbi_set_deadline};

void init_sbi_timer(void)
{
	register_timer_ops(&sbi_timer_ops);
}
