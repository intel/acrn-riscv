/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <acrn/spinlock.h>
#include <acrn/percpu.h>
#include <debug/logmsg.h>
#include <asm/timer.h>
#include <asm/current.h>
#include <asm/atomic.h>

/* buf size should be identical to the size in hvlog option, which is
 * transfered to SOS:
 * bsp/uefi/clearlinux/acrn.conf: hvlog=2M@0x1FE00000
 */

typedef char log_buf_t;
static DEFINE_PER_CPU(log_buf_t[LOG_MESSAGE_MAX_SIZE], logbuf);

struct acrn_logmsg_ctl {
	uint32_t flags;
	atomic_t seq;
	spinlock_t lock;
};

static struct acrn_logmsg_ctl logmsg_ctl = {6};

void init_logmsg(uint32_t flags)
{
	logmsg_ctl.flags = flags;
	atomic_set(&(logmsg_ctl.seq), 0);

	spin_lock_init(&(logmsg_ctl.lock));
}

void do_logmsg(uint32_t severity, const char *fmt, ...)
{
	va_list args;
	uint64_t timestamp, rflags;
	uint16_t pcpu_id;
	char *buffer;

	if (severity > logmsg_ctl.flags)
		return;
	/* Get time-stamp value */
	timestamp = get_tick();

	/* Scale time-stamp appropriately */
	timestamp = ticks_to_us(timestamp);

	/* Get CPU ID */
	pcpu_id = get_processor_id();
	buffer = per_cpu(logbuf, pcpu_id);

	(void)memset(buffer, 0U, LOG_MESSAGE_MAX_SIZE);
	/* Put time-stamp, CPU ID and severity into buffer */
	snprintf(buffer, LOG_MESSAGE_MAX_SIZE, "[%luus][cpu=%hu][sev=%u][seq=%u]:",
			timestamp, pcpu_id, severity, atomic_inc_return(&logmsg_ctl.seq));

	/* Put message into remaining portion of local buffer */
	va_start(args, fmt);
	vsnprintf(buffer + strnlen_s(buffer, LOG_MESSAGE_MAX_SIZE),
		LOG_MESSAGE_MAX_SIZE
		- strnlen_s(buffer, LOG_MESSAGE_MAX_SIZE), fmt, args);
	va_end(args);

	spin_lock_irqsave(&(logmsg_ctl.lock), rflags);

	/* Send buffer to stdout */
	printf("%s\n\r", buffer);

	spin_unlock_irqrestore(&(logmsg_ctl.lock), rflags);

}
