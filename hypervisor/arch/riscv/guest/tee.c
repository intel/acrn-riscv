/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm/current.h>
#include <asm/vm_config.h>
#include <asm/lib/string.h>
#include <asm/guest/vm.h>
#include <asm/guest/virq.h>
#include <logmsg.h>
#include "tee.h"
#include "sbi.h"
#include "rpmi.h"

int is_tee_vm(struct acrn_vm *vm)
{
	return (get_vm_config(vm->vm_id)->guest_flags & GUEST_FLAG_TEE) != 0;
}

int is_ree_vm(struct acrn_vm *vm)
{
	return (get_vm_config(vm->vm_id)->guest_flags & GUEST_FLAG_REE) != 0;
}

void prepare_tee_vm_memmap(struct acrn_vm *vm, const struct acrn_vm_config *vm_config)
{
}

static struct acrn_vm *get_companion_vm(struct acrn_vm *vm)
{
	return get_vm_from_vmid(get_vm_config(vm->vm_id)->companion_vm_id);
}

int32_t tee_answer_ree(struct acrn_vcpu *vcpu)
{
	struct acrn_vm *ree_vm;
	struct acrn_vcpu *ree_vcpu;
	int32_t ret = -EINVAL;
	struct run_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;
	struct cpu_regs *regs = &ctx->cpu_gp_regs.regs;
	uint64_t msg_data_len = regs->a2;
	uint8_t *mm = (uint8_t *)vcpu->mpxy.base;

	ree_vm = get_companion_vm(vcpu->vm);
	ree_vcpu = vcpu_from_vid(ree_vm, vcpu->vcpu_id);

	if (ree_vcpu != NULL) {
		uint8_t *d = (uint8_t *)ree_vcpu->mpxy.base;
		struct run_context *ree_ctx = &ree_vcpu->arch.contexts[ree_vcpu->arch.cur_context].run_ctx;
		struct cpu_regs *ree_regs = &ree_ctx->cpu_gp_regs.regs;
		uint64_t *rc = &ree_regs->a0;
		uint64_t *val = &ree_regs->a1;

		while (d == NULL)  {
			mb();
			d = (uint8_t *)ree_vcpu->mpxy.base;
		}

		memcpy(d, mm + sizeof(uint64_t), msg_data_len - sizeof(uint64_t));
		*rc = SBI_SUCCESS;
		*val = msg_data_len - sizeof(uint64_t);
		ret = 0;
	} else {
		pr_fatal("No REE vCPU running on this pCPU%u, \n", get_pcpu_id());
	}

	return ret;
}


static int32_t tee_switch_to_ree(struct acrn_vcpu *vcpu)
{
	struct acrn_vm *ree_vm;
	struct acrn_vcpu *ree_vcpu;
	int32_t ret = -EINVAL;

	ree_vm = get_companion_vm(vcpu->vm);
	ree_vcpu = vcpu_from_vid(ree_vm, vcpu->vcpu_id);

	if (ree_vcpu != NULL) {
		sleep_thread(&vcpu->thread_obj);
		wake_thread(&ree_vcpu->thread_obj);
		ret = 0;
	} else {
		pr_fatal("No REE vCPU running on this pCPU%u, \n", get_pcpu_id());
	}

	return ret;
}

static int32_t ree_switch_to_tee(struct acrn_vcpu *vcpu)
{
	struct acrn_vm *tee_vm;
	struct acrn_vcpu *tee_vcpu;
	int32_t ret = -EINVAL;
	struct run_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;
	struct cpu_regs *regs = &ctx->cpu_gp_regs.regs;
	uint64_t msg_data_len = regs->a2;
	struct rpmi_mm_request *mm = (struct rpmi_mm_request *)vcpu->mpxy.base;

	tee_vm = get_companion_vm(vcpu->vm);
	tee_vcpu = vcpu_from_vid(tee_vm, vcpu->vcpu_id);
	if (tee_vcpu != NULL) {
		struct rpmi_reqfwd_request *p = (struct rpmi_reqfwd_request *)tee_vcpu->mpxy.base;
		struct run_context *tee_ctx = &tee_vcpu->arch.contexts[tee_vcpu->arch.cur_context].run_ctx;
		struct cpu_regs *tee_regs = &tee_ctx->cpu_gp_regs.regs;
		uint64_t *rc = &tee_regs->a0;
		uint64_t *val = &tee_regs->a1;

		uint8_t *d;
		sleep_thread(&vcpu->thread_obj);

		while (p == NULL)  {
			mb();
			p = (struct rpmi_reqfwd_request *)tee_vcpu->mpxy.base;
		}
		p->retri_status = SBI_SUCCESS;
		p->retri_remain = 0;
		p->retri_returned = msg_data_len;
		*rc = SBI_SUCCESS;
		*val = msg_data_len;

		d = &p->retri_mesg;
		memcpy(d, mm, msg_data_len);
		wake_thread(&tee_vcpu->thread_obj);

		ret = 0;
	} else {
		pr_fatal("No TEE vCPU running on this pCPU%u, \n", get_pcpu_id());
	}

	return ret;
}

#if 0
int32_t hcall_handle_tee_vcpu_boot_done(struct acrn_vcpu *vcpu, __unused struct acrn_vm *target_vm,
		__unused uint64_t param1, __unused uint64_t param2)
{
	struct acrn_vm *ree_vm;
	uint64_t rdi;

	/*
	 * The (RDI == 1) indicates to start REE VM, otherwise only need
	 * to sleep the corresponding TEE vCPU.
	 */
	rdi = vcpu_get_gpreg(vcpu, CPU_REG_RDI);
	if (rdi == 1UL) {
		ree_vm = get_companion_vm(vcpu->vm);
		start_vm(ree_vm);
	}

	sleep_thread(&vcpu->thread_obj);

	return 0;
}
#endif

int32_t tee_switch(struct acrn_vcpu *vcpu)
{
	int32_t ret = 0;

	if (is_tee_vm(vcpu->vm)) {
		ret = tee_switch_to_ree(vcpu);
	} else if (is_ree_vm(vcpu->vm)) {
		ret = ree_switch_to_tee(vcpu);
	}

	return ret;
}
#if 0
void handle_riscv_tee_int(struct ptirq_remapping_info *entry, uint16_t pcpu_id)
{
	struct acrn_vcpu *tee_vcpu;
	struct acrn_vcpu *curr_vcpu = get_running_vcpu(pcpu_id);

	if (is_ree_vm(entry->vm) && is_tee_vm(curr_vcpu->vm)) {
		/*
		 * Non-Secure interrupt (interrupt belongs to REE) comes
		 * when REE vcpu is running, the interrupt will be injected
		 * to REE directly. But when TEE vcpu is running at that time,
		 * we need to inject a predefined vector to TEE for notification
		 * and continue to switch back to TEE for running.
		 */
		tee_vcpu = vcpu_from_pid(get_companion_vm(entry->vm), pcpu_id);
		vlapic_set_intr(tee_vcpu, TEE_FIXED_NONSECURE_VECTOR, LAPIC_TRIG_EDGE);
	} else if (is_tee_vm(entry->vm) && is_ree_vm(curr_vcpu->vm)) {
		/*
		 * Secure interrupt (interrupt belongs to TEE) comes
		 * when TEE vcpu is running, the interrupt will be
		 * injected to TEE directly. But when REE vcpu is running
		 * at that time, we need to switch to TEE for handling,
		 * and copy 0xB20000FF to RDI to notify OPTEE about this.
		 */
		tee_vcpu = vcpu_from_pid(entry->vm, pcpu_id);
		/*
		 * Copy 0xB20000FF to RDI to indicate the switch is from secure interrupt
		 * This is the contract with OPTEE.
		 */
		vcpu_set_gpreg(tee_vcpu, CPU_REG_RDI, OPTEE_FIQ_ENTRY);

		wake_thread(&tee_vcpu->thread_obj);
	} else {
		/* Nothing need to do for this moment */
	}
}
#endif
