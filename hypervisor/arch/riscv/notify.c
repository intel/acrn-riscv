/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <asm/irq.h>
#include <irq.h>
#include <debug/logmsg.h>
#include <asm/per_cpu.h>
#include <asm/notify.h>
#include <asm/smp.h>
#include <asm/current.h>
#include <asm/cpumask.h>
#include <asm/lib/spinlock.h>

static volatile uint64_t smp_call_mask;
spinlock_t smpcall_lock;

/* run in interrupt context */
void kick_notification(void)
{
	uint16_t pcpu_id = get_pcpu_id();
	uint64_t flags;

	spin_lock_irqsave(&smpcall_lock, &flags);
	if (test_bit(pcpu_id, smp_call_mask)) {
		struct smp_call_info_data *smp_call =
			&per_cpu(smp_call_info, pcpu_id);

		if (smp_call->func != NULL) {
			smp_call->func(smp_call->data);
		}
		smp_call->func = NULL;
		smp_call->data = NULL;
		clear_bit(pcpu_id, &smp_call_mask);
	}
	spin_unlock_irqrestore(&smpcall_lock, flags);
}

/* wait until *sync == wake_sync */
void wait_sync_change(volatile const uint64_t *sync, uint64_t wake_sync)
{
	while ((*sync) != wake_sync) {
		cpu_relax();
	}
}

void smp_call_function(uint64_t mask, smp_call_func_t func, void *data)
{
	uint16_t pcpu_id;
	struct smp_call_info_data *smp_call;
	uint64_t flags;

	/* wait for previous smp call complete, which may run on other cpus */
	while (smp_call_mask);
	spin_lock_irqsave(&smpcall_lock, &flags);
	smp_call_mask |= mask;

	pcpu_id = ffs64(mask);
	while (pcpu_id < CONFIG_NR_CPUS) {
		__clear_bit(pcpu_id, &mask);
		if (pcpu_id == get_pcpu_id()) {
			func(data);
			__clear_bit(pcpu_id, &mask);
			__clear_bit(pcpu_id, &smp_call_mask);
		} else if (cpu_online(pcpu_id)) {
			smp_call = &per_cpu(smp_call_info, pcpu_id);
			smp_call->func = func;
			smp_call->data = data;
			smp_ops->send_single_swi(pcpu_id, SMP_FUNC_CALL);
		} else {
			/* pcpu is not in active, print error */
			pr_err("pcpu_id %d not in active!", pcpu_id);
			__clear_bit(pcpu_id, &smp_call_mask);
		}
		pcpu_id = ffs64(mask);
	}
	spin_unlock_irqrestore(&smpcall_lock, flags);
	/* wait for current smp call complete */
	wait_sync_change(&smp_call_mask, 0UL);
}

void smp_call_init(void)
{
	spinlock_init(&smpcall_lock);
	smp_call_mask = 0UL;
}
