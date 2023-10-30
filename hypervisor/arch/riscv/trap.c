/*
 * Copyright (C) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm/cpu.h>
#include "uart.h"
#include "trap.h"

void sexpt_handler()
{
	printk("resv sexpt_handler\n");
}

void sswi_handler()
{
	int cpu = 0;
	char *s = "sswi_handler: d\n";

	s[14] = cpu + '0';
	printk(s);
//	asm volatile ("csrwi sip, 0\n\t"::);
}

void reset_stimer(void)
{
	asm volatile(
		"li a0, 1 \n\t" 	\
		"ecall \n\t"		\
		:::
	);
	return;
}

void stimer_handler(struct cpu_user_regs *regs)
{
//	printk("stimer_handler\n");
	reset_stimer();
	hv_timer_handler(regs);
}

void sexti_handler(struct cpu_user_regs *regs)
{
	dispatch_interrupt(regs);
	printk("sexti_handler\n");
}

static irq_handler_t sirq_handler[] = {
	sexpt_handler,
	sswi_handler,
	sexpt_handler,
	sexpt_handler,
	sexpt_handler,
	stimer_handler,
	sexpt_handler,
	sexpt_handler,
	sexpt_handler,
	sexti_handler,
	sexpt_handler
};

void sint_handler(int irq)
{
	//printk("sint handler\n");
	if (irq < 10)
		sirq_handler[irq]();
	else
		sirq_handler[10]();
}
