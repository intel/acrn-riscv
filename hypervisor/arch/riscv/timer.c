/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <irq.h>
#include <softirq.h>
#include <asm/timer.h>
#include <asm/per_cpu.h>
#include <asm/current.h>
#include <debug/logmsg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <lib/errno.h>
#ifdef CONFIG_MACRN
#include <asm/apicreg.h>
#else
#include <asm/sbi.h>
#endif


unsigned long cpu_khz;  /* CPU clock frequency in kHz. */
unsigned long boot_count;

#define MIN_TIMER_PERIOD_US	500U


/* Qemu default cpu freq is 0x10000000 */
//#define QEMU_CPUFREQ		0x1000000
#define QEMU_CPUFREQ		10000000

struct timer_ops *timer_ops = NULL;

void register_timer_ops(struct timer_ops *ops)
{
	if (!timer_ops)
		timer_ops = ops;
}

static unsigned long get_cpu_khz(void)
{
	return QEMU_CPUFREQ / 1000;
}

uint64_t get_tick(void)
{
	uint64_t tick;

	asm volatile (
		"rdtime %0 \n\t"
			:"=r"(tick):: "memory");
	return tick;
}

/* Return number of nanoseconds since boot */
uint64_t get_s_time(void)
{
	uint64_t ticks = get_tick();
	return ticks_to_us(ticks);
}

void preinit_timer(void)
{
	cpu_khz = get_cpu_khz();
	boot_count = get_tick();
	pr_dbg("cpu_khz = %ld boot_count=%ld \r\n", cpu_khz, boot_count);

	if (!timer_ops) {
#ifdef CONFIG_MACRN
		init_clint_timer();
#else
		init_sbi_timer();
#endif
	}

	return timer_ops->preinit_timer();
}

int set_deadline(uint64_t deadline)
{
	uint64_t ticks = get_tick();

	if (deadline < ticks) {
		pr_dbg("deadline not correct");
		deadline = ticks + us_to_ticks(MIN_TIMER_PERIOD_US);
	}

	if (!timer_ops)
		return -ENODEV;
	else
		return timer_ops->set_deadline(deadline);
}

void udelay(uint32_t us)
{
	uint64_t deadline;

	deadline = us_to_ticks(us) + get_tick();
	while (get_tick() < deadline);
	dsb();
	isb();
}

void update_physical_timer(struct per_cpu_timers *cpu_timer)
{
	struct hv_timer *timer = NULL;

	/* find the next event timer */
	if (!list_empty(&cpu_timer->timer_list)) {
		timer = container_of((&cpu_timer->timer_list)->next,
			struct hv_timer, node);

		/* it is okay to program a expired time */
		set_deadline(timer->timeout);
	}
}

void hv_timer_handler(void)
{
	fire_softirq(SOFTIRQ_TIMER);
}

/* run in interrupt context */
static void timer_expired_handler(__unused uint32_t irq, __unused void *data)
{
	fire_softirq(SOFTIRQ_TIMER);
}

#define TIMER_IRQ 7
void init_hw_timer(void)
{
	int32_t retval = 0;

	return;
	if (get_pcpu_id() == BSP_CPU_ID) {
		retval = request_irq(TIMER_IRQ, timer_expired_handler, NULL, IRQF_NONE);
		if (retval < 0) {
			pr_err("Timer setup failed");
		}
	}
}

uint64_t cpu_ticks(void)
{
	return get_tick();
}

uint32_t cpu_tickrate(void)
{
	return cpu_khz;
}
