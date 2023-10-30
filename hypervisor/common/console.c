/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <common/uart16550.h>
#include <asm/early_printk.h>
#include <acrn/time.h>
#include <asm/io.h>
#include <debug/logmsg.h>
#include <console.h>
#include <common/shell.h>

struct hv_timer console_timer;
bool console_enabled = false;

#define CONSOLE_KICK_TIMER_TIMEOUT  4000UL /* timeout is 4ms */

void console_init(void)
{
#ifndef RUN_ON_QEMU
	uart16550_init(true);
	console_enabled = true;
#endif
	shell_init();
}

void console_putc(const char *ch)
{
	if (console_enabled) {
		uart16550_putc(*ch);
		early_putch(*ch);
		early_flush();
	} else {
		early_putch(*ch);
		early_flush();
	}
}


size_t console_write(const char *s, size_t len)
{
	while (len--)
	{
		console_putc(s++);
	}
	return 0;

}

char console_getc(void)
{
	if (console_enabled) {
		return uart16550_getc();
	} else {
#ifdef RUN_ON_QEMU
		return early_getch();
#else
		return 'f';
#endif
	}
}

static void console_timer_callback(__unused void *data)
{
	/* Kick HV-Shell and Uart-Console tasks */
	shell_kick();
}

void console_setup_timer(void)
{
	uint64_t period_in_tick, fire_tick;

	period_in_tick = us_to_ticks(CONSOLE_KICK_TIMER_TIMEOUT);
	fire_tick = get_tick() + period_in_tick;
	initialize_timer(&console_timer,
			console_timer_callback, NULL,
			fire_tick, TICK_MODE_PERIODIC, period_in_tick);

	/* Start an periodic timer */
	if (add_timer(&console_timer) != 0) {
		pr_dbg("Failed to add console kick timer");
	}
	pr_dbg("add timer tick= %lx", period_in_tick);
}

void suspend_console(void)
{
	del_timer(&console_timer);
}

void resume_console(void)
{
	console_setup_timer();
}
