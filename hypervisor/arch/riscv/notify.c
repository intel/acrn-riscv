/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <public/irq.h>
#include <debug/logmsg.h>
#include <acrn/percpu.h>
#include <asm/notify.h>
#include <asm/current.h>

static cpumask_t smp_call_mask;
spinlock_t smpcall_lock;

static DEFINE_PER_CPU(struct smp_call_info_data, smp_call_info);

/* run in interrupt context */
void kick_notification(void)
{
	/* Notification vector is used to kick taget cpu out of non-root mode.
	 * And it also serves for smp call.
	 */
	uint16_t pcpu_id = get_processor_id();

	if (test_bit(pcpu_id, &smp_call_mask)) {
		struct smp_call_info_data *smp_call =
			&per_cpu(smp_call_info, pcpu_id);

		if (smp_call->func != NULL) {
			smp_call->func(smp_call->data);
		}
		clear_bit(pcpu_id, &smp_call_mask);
	}
}

/* wait until *sync == wake_sync */
void wait_sync_change(volatile const uint64_t *sync, uint64_t wake_sync)
{
	while ((*sync) != wake_sync) {
		cpu_relax();
	}
}

void smp_call_function(cpumask_t mask, smp_call_func_t func, void *data)
{
	uint16_t pcpu_id;
	struct smp_call_info_data *smp_call;

	spin_lock(&smpcall_lock);
	/* wait for previous smp call complete, which may run on other cpus */
	while (!cpumask_empty(&smp_call_mask));
	cpumask_or(&smp_call_mask, &smp_call_mask, &mask);

	pcpu_id = 0;
	while (pcpu_id < CONFIG_NR_CPUS) {
		__clear_bit(pcpu_id, &mask);
		if (cpu_online(pcpu_id)) {
			smp_call = &per_cpu(smp_call_info, pcpu_id);
			smp_call->func = func;
			smp_call->data = data;
		} else {
			/* pcpu is not in active, print error */
			pr_err("pcpu_id %d not in active!", pcpu_id);
			__clear_bit(pcpu_id, &smp_call_mask);
		}
		pcpu_id += 1U;
	}
	send_dest_ipi_mask(smp_call_mask, NOTIFY_VCPU_SWI);
	/* wait for current smp call complete */
	//wait_sync_change(&smp_call_mask, 0UL);
	spin_unlock(&smpcall_lock);
}

/*
 * only run bsp.
 * */

void smp_call_init(void)
{
	spin_lock_init(&smpcall_lock);
}

/**
 * @brief Check if the NMI is for notification purpose
 *
 * @return true, if the NMI is triggered for notifying vCPU
 * @return false, if the NMI is triggered for other purpose
 */
bool is_notification_nmi(const struct acrn_vm *vm)
{
	return false;
}
