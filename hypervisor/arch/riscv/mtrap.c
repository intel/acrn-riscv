/*
 * Copyright (C) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm/cpu.h>
#include <asm/timer.h>
#include "uart.h"
#include "trap.h"
#include "kernel.h"

static int cpu_id()
{
	int cpu;

	asm volatile (
		"csrr a0, mhartid \n\t" \
		"mv %0, a0 \n\t" \
		:"=r"(cpu):
	);

	return cpu;
}

void m_service(struct cpu_regs *regs)
{
	int call = regs->a0;
	switch (call) {
		case 1:
			asm volatile(
				"li t0, 0x20 \n\t" \
				"csrc mip, t0 \n\t"
				::: "memory"
			);
			regs->ip += 4;
			break;
		default:
			break;
	}
}

void mexpt_handler()
{
//	printk("resv mexpt_handler\n");
}

void mswi_handler()
{
	int cpu = cpu_id();
	char *s = "mswi_handler: d\n";

	s[14] = cpu + '0';
	printk(s);
	cpu *= 4;
	cpu += 0x02000000;

	asm volatile (
		"li t0, 0 \n\t" \
		"sw t0, 0(%0) \n\t" \
		"li t0, 0x2 \n\t" \
		"csrw mip, t0\n\t"
		:: "r"(cpu)
	);
}

void mtimer_handler()
{
	int cpu = cpu_id();
	uint64_t val = 0x20;
	uint64_t addr = CLINT_MTIMECMP(cpu);

	asm volatile (
		"csrs mip, %0 \n\t" \
		"sw %2, 0(%1)"
		:: "r"(val), "r"(addr), "r"(CLINT_DISABLE_TIMER): "memory"
	);
//	printk("mtimer_handler\n");
}

void mexti_handler()
{
//	printk("mexti_handler\n");
}

void test_swi()
{
	printk("trigger swi\n");
	asm volatile (
		"li a0, 0x1 \n\t" \
		"li a1, 0x02000000 \n\t" \
		"sw a0, 0(a1) \n\t"
		::
	);
}

typedef void (* irq_handler_t)(void);
static irq_handler_t mirq_handler[] = {
	mexpt_handler,
	mexpt_handler,
	mexpt_handler,
	mswi_handler,
	mexpt_handler,
	mexpt_handler,
	mexpt_handler,
	mtimer_handler,
	mexpt_handler,
	mexpt_handler,
	mexpt_handler,
	mexti_handler,
	mexpt_handler
};

void mint_handler(int irq)
{
//	printk("mint_handler\n");
	if (irq < 12)
		mirq_handler[irq]();
	else
		mirq_handler[12]();
/*
	asm volatile (
		"csrw mepc, %0\n\t" \
		"mret" :: "r"(kernel)
	);
*/
}
