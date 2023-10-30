/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */
#include <schedule.h>
#include <asm/current.h>
#include <event.h>
#include <logmsg.h>

void init_event(struct sched_event *event)
{
	spin_lock_init(&event->lock);
	event->set = false;
	event->waiting_thread = NULL;
}

void reset_event(struct sched_event *event)
{
	uint64_t rflag;

	spin_lock_irqsave(&event->lock, rflag);
	event->set = false;
	event->waiting_thread = NULL;
	spin_unlock_irqrestore(&event->lock, rflag);
}

/* support exclusive waiting only */
void wait_event(struct sched_event *event)
{
	uint64_t rflag;

	spin_lock_irqsave(&event->lock, rflag);
	event->waiting_thread = sched_get_current(get_processor_id());
	while (!event->set && (event->waiting_thread != NULL)) {
		sleep_thread(event->waiting_thread);
		spin_unlock_irqrestore(&event->lock, rflag);
		schedule();
		spin_lock_irqsave(&event->lock, rflag);
	}
	event->set = false;
	event->waiting_thread = NULL;
	spin_unlock_irqrestore(&event->lock, rflag);
}

void signal_event(struct sched_event *event)
{
	uint64_t rflag;

	spin_lock_irqsave(&event->lock, rflag);
	event->set = true;
	if (event->waiting_thread != NULL) {
		wake_thread(event->waiting_thread);
	}
	spin_unlock_irqrestore(&event->lock, rflag);
}
