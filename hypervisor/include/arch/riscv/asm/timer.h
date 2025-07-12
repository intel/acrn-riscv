/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_TIMER_H__
#define __RISCV_TIMER_H__

#include <asm/cpu.h>
#include <asm/system.h>
#include <list.h>
#include <timer.h>

struct timer_ops {
	void (*preinit_timer)(void);
	uint64_t (*get_tick)(void);
	int (*set_deadline)(uint64_t deadline);
};

extern void register_timer_ops(struct timer_ops *ops);
extern struct timer_ops *timer_ops;

typedef uint64_t cycles_t;

static inline cycles_t get_cycles (void)
{
        isb();
        return cpu_csr_read(scounter);
}

extern uint64_t boot_count;

extern void udelay(uint32_t us);
extern uint64_t get_tick(void);
extern void preinit_timer(void);
extern void update_physical_timer(struct per_cpu_timers *cpu_timer);
extern int set_deadline(uint64_t deadline);

#define CYCLES_PER_MS	us_to_ticks(1000U)

#define CLINT_MTIME		(CONFIG_CLINT_BASE + 0xBFF8)
#define CLINT_MTIMECMP(cpu)	(CONFIG_CLINT_TM_BASE + (cpu * 0x8))
#define CLINT_DISABLE_TIMER	0xffffffffffffffff

#endif /* __RISCV_TIMER_H__ */
