/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#include <asm/current.h>
#include <schedule.h>
#include <acrn/lib.h>
#include <asm/cpu.h>
#include <asm/guest/vcpu.h>
#include <logmsg.h>
#include <trace.h>

void vcpu_thread(struct thread_object *obj)
{
	struct acrn_vcpu *vcpu = container_of(obj, struct acrn_vcpu, thread_obj);
	uint32_t basic_exit_reason = 0U;
	int32_t ret = 0;

	do {
		if (!is_lapic_pt_enabled(vcpu)) {
			CPU_IRQ_DISABLE();
		}

		/* Don't open interrupt window between here and vmentry */
		if (need_reschedule(pcpuid_from_vcpu(vcpu))) {
			schedule();
		}

		/* Check and process pending requests(including interrupt) */
		ret = acrn_handle_pending_request(vcpu);
		if (ret < 0) {
			pr_fatal("vcpu handling pending request fail");
			get_vm_lock(vcpu->vm);
			zombie_vcpu(vcpu, VCPU_ZOMBIE);
			put_vm_lock(vcpu->vm);
			/* Fatal error happened (triple fault). Stop the vcpu running. */
			continue;
		}

		reset_event(&vcpu->events[VCPU_EVENT_VIRTUAL_INTERRUPT]);
		profiling_vmenter_handler(vcpu);

//		TRACE_2L(TRACE_VM_ENTER, 0UL, 0UL);
		ret = run_vcpu(vcpu);
		if (ret != 0) {
			pr_fatal("vcpu resume failed");
			get_vm_lock(vcpu->vm);
			zombie_vcpu(vcpu, VCPU_ZOMBIE);
			put_vm_lock(vcpu->vm);
			/* Fatal error happened (resume vcpu failed). Stop the vcpu running. */
			continue;
		}
		basic_exit_reason = vcpu->arch.exit_reason & 0xFFFFU;
		//TRACE_2L(TRACE_VM_EXIT, basic_exit_reason, vcpu_get_rip(vcpu));

		vcpu->arch.nrexits++;

		profiling_pre_vmexit_handler(vcpu);

		if (!is_lapic_pt_enabled(vcpu)) {
			CPU_IRQ_ENABLE();
		}
		/* Dispatch handler */
		ret = vmexit_handler(vcpu);
		if (ret < 0) {
			pr_fatal("dispatch VM exit handler failed for reason"
				" %d, ret = %d!", basic_exit_reason, ret);
			vcpu_inject_gp(vcpu, 0U);
			continue;
		}

		profiling_post_vmexit_handler(vcpu);
	} while (1);
}

void default_idle(__unused struct thread_object *obj)
{
	uint16_t pcpu_id = get_processor_id();

	while (1) {
		if (need_reschedule(pcpu_id)) {
			pr_dbg("%s %d______", __func__, __LINE__);
			schedule();
		} else {
			cpu_do_idle();
		}
	}
}

void run_idle_thread(void)
{
	uint16_t pcpu_id = get_processor_id();
	struct thread_object *idle = &idle_vcpu[pcpu_id].thread_obj;
	char idle_name[16];

	snprintf(idle_name, 16U, "idle%hu", pcpu_id);
	(void)strncpy_s(idle->name, 16U, idle_name, 16U);
	idle->pcpu_id = pcpu_id;
	idle->thread_entry = default_idle;
	idle->switch_out = NULL;
	idle->switch_in = NULL;

	run_thread(idle);

	/* Control should not come here */
	cpu_dead();
}
