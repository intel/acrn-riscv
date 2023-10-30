/*
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
*/
#ifndef EVENT_H
#define EVENT_H
#include <acrn/spinlock.h>

struct sched_event {
	spinlock_t lock;
	bool set;
	struct thread_object* waiting_thread;
};

void init_event(struct sched_event *event);
void reset_event(struct sched_event *event);
void wait_event(struct sched_event *event);
void signal_event(struct sched_event *event);

#endif /* EVENT_H */
