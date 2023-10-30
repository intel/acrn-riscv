/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

//#include <rtl.h>
#include <list.h>
#include <asm/system.h>
#include <acrn/percpu.h>
#include <asm/current.h>
#include <schedule.h>
#ifdef CONFIG_ARM
#include <asm/gic.h>
#elif CONFIG_RISCV
#include <asm/irq.h>
#else
#include <acrn/bitops.h>
#endif
#include <debug/logmsg.h>
#include <asm/guest/vcpu.h>

//DEFINE_PER_CPU(struct thread_object, idle);
DEFINE_PER_CPU(struct sched_control, sched_ctl);

bool is_idle_thread(const struct thread_object *obj)
{
	uint16_t pcpu_id = obj->pcpu_id;
	return (obj == &idle_vcpu[pcpu_id].thread_obj);
}

static inline bool is_blocked(const struct thread_object *obj)
{
	return obj->status == THREAD_STS_BLOCKED;
}

static inline bool is_running(const struct thread_object *obj)
{
	return obj->status == THREAD_STS_RUNNING;
}

static inline void set_thread_status(struct thread_object *obj, enum thread_object_state status)
{
	obj->status = status;
}

void obtain_schedule_lock(uint16_t pcpu_id, uint64_t *rflag)
{
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);
	spin_lock_irqsave(&ctl->scheduler_lock, *rflag);
}

void release_schedule_lock(uint16_t pcpu_id, uint64_t rflag)
{
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);
	spin_unlock_irqrestore(&ctl->scheduler_lock, rflag);
}

static struct acrn_scheduler *get_scheduler(uint16_t pcpu_id)
{
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);
	return ctl->scheduler;
}

/**
 * @pre obj != NULL
 */
uint16_t sched_get_pcpuid(const struct thread_object *obj)
{
	return obj->pcpu_id;
}

void init_sched(uint16_t pcpu_id)
{
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);

	spin_lock_init(&ctl->scheduler_lock);
	ctl->flags = 0UL;
	ctl->curr_obj = NULL;
	ctl->pcpu_id = pcpu_id;
#ifdef CONFIG_SCHED_NOOP
	ctl->scheduler = &sched_noop;
#endif
#ifdef CONFIG_SCHED_IORR
	ctl->scheduler = &sched_iorr;
#endif
#ifdef CONFIG_SCHED_BVT
	ctl->scheduler = &sched_bvt;
#endif

	if (ctl->scheduler->init != NULL) {
		ctl->scheduler->init(ctl);
	}
}

void deinit_sched(uint16_t pcpu_id)
{
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);

	if (ctl->scheduler->deinit != NULL) {
		ctl->scheduler->deinit(ctl);
	}
}

void init_thread_data(struct thread_object *obj)
{
	struct acrn_scheduler *scheduler = get_scheduler(obj->pcpu_id);
	volatile uint64_t rflag;

	obtain_schedule_lock(obj->pcpu_id, &rflag);
	if (scheduler->init_data != NULL) {
		scheduler->init_data(obj);
	}
	/* initial as BLOCKED status, so we can wake it up to run */
	set_thread_status(obj, THREAD_STS_BLOCKED);
	release_schedule_lock(obj->pcpu_id, rflag);
}

void deinit_thread_data(struct thread_object *obj)
{
	struct acrn_scheduler *scheduler = get_scheduler(obj->pcpu_id);

	if (scheduler->deinit_data != NULL) {
		scheduler->deinit_data(obj);
	}
}

struct thread_object *sched_get_current(uint16_t pcpu_id)
{
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);
	return ctl->curr_obj;
}

/**
 * @pre delmode == DEL_MODE_IPI || delmode == DEL_MODE_NMI
 */
void make_reschedule_request(uint16_t pcpu_id, uint16_t delmode)
{
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);

	set_bit(NEED_RESCHEDULE, &ctl->flags);
	if (get_processor_id() != pcpu_id) {
		switch (delmode) {
#if 0
		case DEL_MODE_IPI:
			send_single_ipi(pcpu_id, NOTIFY_VCPU_VECTOR);
			break;
		case DEL_MODE_NMI:
			send_single_nmi(pcpu_id);
			break;
#endif
		case DEL_MODE_SGI:
			pr_dbg("%s, %d___", __func__, __LINE__);
//			send_sgi_one(pcpu_id, GIC_SGI_EVENT_CHECK);
			break;
		case DEL_MODE_SWI:
			pr_dbg("%s, %d___", __func__, __LINE__);
			send_single_swi(pcpu_id, CLINT_SWI_EVENT_CHECK);
			break;
		default:
			pr_fatal("%s Unknown delivery mode %u for pCPU%u", __func__, delmode, pcpu_id);
			break;
		}
	}
}

bool need_reschedule(uint16_t pcpu_id)
{
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);

	return test_bit(NEED_RESCHEDULE, &ctl->flags);
}

void schedule(void)
{
	uint16_t pcpu_id = get_processor_id();
	struct sched_control *ctl = &per_cpu(sched_ctl, pcpu_id);
	struct thread_object *next = &idle_vcpu[pcpu_id].thread_obj;
	struct thread_object *prev = ctl->curr_obj;
	uint64_t rflag;

	obtain_schedule_lock(pcpu_id, &rflag);
	if (ctl->scheduler->pick_next != NULL) {
		next = ctl->scheduler->pick_next(ctl);
	}
	clear_bit(NEED_RESCHEDULE, &ctl->flags);

	/* If we picked different sched object, switch context */
	if (prev != next) {
		if (prev != NULL) {
			if (prev->switch_out != NULL) {
				prev->switch_out(prev);
			}
			set_thread_status(prev, prev->be_blocking ? THREAD_STS_BLOCKED : THREAD_STS_RUNNABLE);
			prev->be_blocking = false;
		}

		if (next->switch_in != NULL) {
			next->switch_in(next);
		}
		set_thread_status(next, THREAD_STS_RUNNING);

		ctl->curr_obj = next;
		release_schedule_lock(pcpu_id, rflag);
		set_current(next);
		current->pcpu_id = pcpu_id;
		arch_switch_to(&prev->host_sp, &next->host_sp);
	} else {
		release_schedule_lock(pcpu_id, rflag);
	}
}

void sleep_thread(struct thread_object *obj)
{
	uint16_t pcpu_id = obj->pcpu_id;
	struct acrn_scheduler *scheduler = get_scheduler(pcpu_id);
	uint64_t rflag;

	obtain_schedule_lock(pcpu_id, &rflag);
	if (scheduler->sleep != NULL) {
		scheduler->sleep(obj);
	}
	if (is_running(obj)) {
		if (obj->notify_mode == SCHED_NOTIFY_NMI) {
			make_reschedule_request(pcpu_id, DEL_MODE_NMI);
		} else if (obj->notify_mode == SCHED_NOTIFY_IPI) {
			make_reschedule_request(pcpu_id, DEL_MODE_IPI);
		} else {
			make_reschedule_request(pcpu_id, DEL_MODE_SGI);
		}
	}
	obj->be_blocking = true;
	release_schedule_lock(pcpu_id, rflag);
}

void sleep_thread_sync(struct thread_object *obj)
{
	sleep_thread(obj);
	while (!is_blocked(obj)) {
		wfi();
	}
}

void wake_thread(struct thread_object *obj)
{
	uint16_t pcpu_id = obj->pcpu_id;
	struct acrn_scheduler *scheduler;
	uint64_t rflag;

	obtain_schedule_lock(pcpu_id, &rflag);
	if (is_blocked(obj)) {
		scheduler = get_scheduler(pcpu_id);
		if (scheduler->wake != NULL) {
			scheduler->wake(obj);
		}
		set_thread_status(obj, THREAD_STS_RUNNABLE);
		if (obj->notify_mode == SCHED_NOTIFY_IPI) {
			make_reschedule_request(pcpu_id, DEL_MODE_IPI);
		} else {
			make_reschedule_request(pcpu_id, DEL_MODE_SGI);
		}
	}
	release_schedule_lock(pcpu_id, rflag);
}

void yield_current(void)
{
#if 0
	if (obj->notify_mode == SCHED_NOTIFY_IPI) {
		make_reschedule_request(get_processor_id(), DEL_MODE_IPI);
	} else {
#endif
		make_reschedule_request(get_processor_id(), DEL_MODE_SGI);
//	}
}

void run_thread(struct thread_object *obj)
{
	uint64_t rflag;

	init_thread_data(obj);
	obtain_schedule_lock(obj->pcpu_id, &rflag);
	get_cpu_var(sched_ctl).curr_obj = obj;
	set_thread_status(obj, THREAD_STS_RUNNING);
	release_schedule_lock(obj->pcpu_id, rflag);

	if (obj->thread_entry != NULL) {
		obj->thread_entry(obj);
	}
}
