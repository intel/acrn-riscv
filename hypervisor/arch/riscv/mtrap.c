/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <asm/cpu.h>
#include <asm/timer.h>
#include <asm/current.h>
#include <asm/notify.h>
#include <asm/per_cpu.h>
#include <asm/notify.h>
#include <asm/irq.h>
#include <asm/lib/bits.h>
#include <debug/logmsg.h>
#include <softirq.h>
#include "uart.h"
#include "trap.h"

static int cpu_id(void)
{
	int cpu;

	asm volatile (
		"csrr %0, mhartid \n\t"
		:"=r"(cpu)::
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
				::: "memory", "t0"
			);
			regs->ip += 4;
			break;
		default:
			break;
	}
}

static void mexpt_handler(void)
{
}

static void mswi_handler(void)
{
	int cpu = cpu_id();
	uint64_t off = CLINT_SWI_REG;
#if 0
	char *s = "mswi_handler: d\n";

	s[14] = cpu + '0';
	early_printk(s);
#endif

	off += (uint64_t)(cpu * 4);

	asm volatile (
		"sw x0, 0(%0) \n\t"
		:: "r"(off): "memory"
	);

	if (test_bit(NOTIFY_VCPU_SWI, per_cpu(swi_vector, cpu).type))
		clear_bit(NOTIFY_VCPU_SWI, &(per_cpu(swi_vector, cpu).type));

	if (test_bit(SMP_FUNC_CALL, per_cpu(swi_vector, cpu).type)) {
		clear_bit(SMP_FUNC_CALL, &(per_cpu(swi_vector, cpu).type));
		kick_notification();
	}
}

static void mtimer_handler(void)
{
	int cpu = cpu_id();
	uint64_t val = 0x80;
	uint64_t addr = CLINT_MTIMECMP(cpu);

	asm volatile (
		"sw %1, 0(%0) \n\t"
		"csrc mip, %2"
		:: "r"(addr), "r"(CLINT_DISABLE_TIMER), "r"(val): "memory"
	);
#ifdef CONFIG_MACRN
	hv_timer_handler();
#endif
}

static void mexti_handler(void)
{
	handle_mexti();
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
	ASSERT(current != 0);
	if (irq < 12)
		mirq_handler[irq]();
	else
		mirq_handler[12]();

	do_softirq();
}
