/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_PERCPU_H__
#define __RISCV_PERCPU_H__

#ifndef __ASSEMBLY__

#include <asm/smp.h>
#include <asm/notify.h>
#include <asm/config.h>
#include <asm/vm_config.h>
#include <asm/guest/vcpu.h>
#include <logmsg.h>
#include <types.h>
#include <irq.h>
#include <schedule.h>
#include <timer.h>

struct per_cpu_region {
	struct shared_buf *sbuf[ACRN_SBUF_PER_PCPU_ID_MAX];
	char logbuf[LOG_MESSAGE_MAX_SIZE];
	uint32_t npk_log_ref;
	uint64_t irq_count[NR_IRQS];
	uint64_t softirq_pending;
	uint32_t softirq_servicing;
	struct list_head softirq_dev_entry_list;
	struct swi_vector swi_vector;
	struct acrn_vcpu *vcpu_array[CONFIG_MAX_VM_NUM];
	struct acrn_vcpu *ever_run_vcpu;
	void *vcpu_run;
	struct sched_control sched_ctl;
	uint32_t lapic_id;
	struct smp_call_info_data smp_call_info;
	uint32_t cpu_id;
	struct per_cpu_timers cpu_timers;
	struct thread_object idle;
	uint32_t mode_to_kick_pcpu;
	uint32_t mode_to_idle;
	struct sched_noop_control sched_noop_ctl;
	struct sched_iorr_control sched_iorr_ctl;
	struct sched_bvt_control sched_bvt_ctl;
	struct sched_prio_control sched_prio_ctl;
} __aligned(PAGE_SIZE); /* per_cpu_region size aligned with PAGE_SIZE */

extern struct per_cpu_region per_cpu_data[NR_CPUS];
#define per_cpu(name, pcpu_id)	\
	(per_cpu_data[(pcpu_id)].name)
#define get_cpu_var(name)	per_cpu(name, get_pcpu_id())

extern void init_percpu_areas(void);
#endif /* !__ASSEMBLY__ */

#endif /* __RISCV_PERCPU_H__ */
