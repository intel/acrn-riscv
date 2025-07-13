/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <errno.h>
#include <asm/vmx.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/virq.h>
#include <asm/guest/vm.h>
#include <asm/guest/vmexit.h>
#include <asm/guest/vio.h>
#include <asm/guest/s2vm.h>
#include <asm/guest/vcsr.h>
#include <trace.h>
#include <logmsg.h>

static int32_t unhandled_vmexit_handler(struct acrn_vcpu *vcpu)
{
	pr_fatal("Error: Unhandled VM exit condition from guest at 0x%016lx ",
			vcpu_get_gpreg(vcpu, CPU_REG_IP));

	pr_fatal("Exit Reason: 0x%016lx ", vcpu->arch.exit_reason);
	return 0;
}

static int32_t hlt_vmexit_handler(struct acrn_vcpu *vcpu)
{
	if ((vcpu->arch.pending_req == 0UL) && (!vclint_has_pending_intr(vcpu))) {
		wait_event(&vcpu->events[VCPU_EVENT_VIRTUAL_INTERRUPT]);
	}
	return 0;
}

/*
int32_t ecall_vmexit_handler(struct acrn_vcpu *vcpu)
{
	uint64_t rax, rbx, rcx, rdx;

	rax = vcpu_get_gpreg(vcpu, CPU_REG_A0);
	rbx = vcpu_get_gpreg(vcpu, CPU_REG_A1);
	rcx = vcpu_get_gpreg(vcpu, CPU_REG_A2);
	rdx = vcpu_get_gpreg(vcpu, CPU_REG_A3);
	guest_cpuid(vcpu, (uint32_t *)&rax, (uint32_t *)&rbx,
		(uint32_t *)&rcx, (uint32_t *)&rdx);
	vcpu_set_gpreg(vcpu, CPU_REG_A0, rax);
	vcpu_set_gpreg(vcpu, CPU_REG_A1, rbx);
	vcpu_set_gpreg(vcpu, CPU_REG_A2, rcx);
	vcpu_set_gpreg(vcpu, CPU_REG_A3, rdx);

	return 0;
}
*/

bool has_rt_vm(void)
{
	return false;
}

bool is_rt_vm(const struct acrn_vm *vm)
{
	return false;
}

/* vmexit handler for just injecting a #UD exception
 *
 * ACRN doesn't support nested virtualization, the following VMExit will inject #UD
 * VMCLEAR/VMLAUNCH/VMPTRST/VMREAD/VMRESUME/VMWRITE/HXOFF/HXON.
 * ACRN doesn't enable VMFUNC, VMFUNC treated as undefined.
 */
static int32_t undefined_vmexit_handler(struct acrn_vcpu *vcpu)
{
	vcpu_inject_ud(vcpu);
	return 0;
}

static int32_t pf_load_vmexit_handler(struct acrn_vcpu *vcpu)
{
	pr_info("%s\n", __func__);
	return 0;
}

static int32_t pf_store_vmexit_handler(struct acrn_vcpu *vcpu)
{
	pr_info("%s\n", __func__);
	return 0;
}

static int32_t pf_ins_vmexit_handler(struct acrn_vcpu *vcpu)
{
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;
	struct cpu_regs *regs = &ctx->cpu_gp_regs.regs;

	pr_info("%s: %lx\n", __func__, regs->ip);
	return 0;
}

/* VM Dispatch table for Exit condition handling */
#ifdef CONFIG_MACRN
static int32_t mswi_vmexit_handler(struct acrn_vcpu *vcpu)
{
	return 0;
}

static int32_t mti_vmexit_handler(struct acrn_vcpu *vcpu)
{
	ASSERT(current != 0);
	return 0;
}

/* VM Dispatch table for Exit condition handling */
static const struct vm_exit_dispatch interrupt_dispatch_table[NR_HX_EXIT_IRQ_REASONS] = {
	[HX_EXIT_IRQ_RSV] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_SSWI] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_VIRT_SSWI] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_MSWI] = {
		.handler = mswi_vmexit_handler},
	[HX_EXIT_IRQ_STIMER] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_VSTIMER] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_MTIMER] = {
		.handler = mti_vmexit_handler},
	[HX_EXIT_IRQ_SEXT] = {
		.handler = external_interrupt_vmexit_handler},
	[HX_EXIT_IRQ_VSEXT] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_MEXT] = {
		.handler = mexti_vmexit_handler},
	[HX_EXIT_IRQ_GUEST_SEXT] = {
		.handler = unhandled_vmexit_handler},
};

static const struct vm_exit_dispatch exception_dispatch_table[NR_HX_EXIT_REASONS] = {
	[HX_EXIT_INS_MISALIGN] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_INS_ACCESS] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_INS_ILLEGAL] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_BREAKPOINT] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_LOAD_MISALIGN] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_LOAD_ACCESS] = {
		.handler = mmio_access_vmexit_handler},
	[HX_EXIT_STORE_MISALIGN] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_STORE_ACCESS] = {
		.handler = mmio_access_vmexit_handler},
	[HX_EXIT_ECALL_U] = {
		.handler = unhandled_vmexit_handler},
	[HX_EXIT_ECALL_HS] = {
		.handler = vmcall_vmexit_handler},
	[HX_EXIT_ECALL_VS] = {
		.handler = unhandled_vmexit_handler},
	[HX_EXIT_ECALL_M] = {
		.handler = unhandled_vmexit_handler},
	[HX_EXIT_PF_INS] = {
		.handler = pf_ins_vmexit_handler},
	[HX_EXIT_PF_LOAD] = {
		.handler = pf_load_vmexit_handler},
	[HX_EXIT_RESV] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_PF_STORE] = {
		.handler = pf_store_vmexit_handler},
	[HX_EXIT_REASON_INVLPG] = {
		.handler = unhandled_vmexit_handler},
	[HX_EXIT_PF_GUEST_INS] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_PF_GUEST_LOAD] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_VIRT_INS] = {
		.handler = hlt_vmexit_handler},
	[HX_EXIT_PF_GUEST_STORE] = {
		.handler = undefined_vmexit_handler},
};

#else /* !CONFIG_MACRN */
static int32_t sswi_vmexit_handler(struct acrn_vcpu *vcpu)
{
	return 0;
}

static int32_t sti_vmexit_handler(struct acrn_vcpu *vcpu)
{
	ASSERT(current != 0);
	return 0;
}

/* VM Dispatch table for Exit condition handling */
static const struct vm_exit_dispatch interrupt_dispatch_table[NR_HX_EXIT_IRQ_REASONS] = {
	[HX_EXIT_IRQ_RSV] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_SSWI] = {
		.handler = sswi_vmexit_handler},
	[HX_EXIT_IRQ_VIRT_SSWI] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_MSWI] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_STIMER] = {
		.handler = sti_vmexit_handler},
	[HX_EXIT_IRQ_VSTIMER] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_MTIMER] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_SEXT] = {
		.handler = external_interrupt_vmexit_handler},
	[HX_EXIT_IRQ_VSEXT] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_IRQ_MEXT] = {
		.handler = mexti_vmexit_handler},
	[HX_EXIT_IRQ_GUEST_SEXT] = {
		.handler = unhandled_vmexit_handler},
};

static const struct vm_exit_dispatch exception_dispatch_table[NR_HX_EXIT_REASONS] = {
	[HX_EXIT_INS_MISALIGN] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_INS_ACCESS] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_INS_ILLEGAL] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_BREAKPOINT] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_LOAD_MISALIGN] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_STORE_MISALIGN] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_STORE_ACCESS] = {
		.handler = exception_vmexit_handler},
	[HX_EXIT_ECALL_U] = {
		.handler = unhandled_vmexit_handler},
	[HX_EXIT_ECALL_HS] = {
		.handler = unhandled_vmexit_handler},
	[HX_EXIT_ECALL_VS] = {
		.handler = vmcall_vmexit_handler},
	[HX_EXIT_ECALL_M] = {
		.handler = unhandled_vmexit_handler},
	[HX_EXIT_PF_INS] = {
		.handler = pf_ins_vmexit_handler},
	[HX_EXIT_PF_LOAD] = {
		.handler = pf_load_vmexit_handler},
	[HX_EXIT_RESV] = {
		.handler = undefined_vmexit_handler},
	[HX_EXIT_PF_STORE] = {
		.handler = pf_store_vmexit_handler},
	[HX_EXIT_REASON_INVLPG] = {
		.handler = unhandled_vmexit_handler},
	[HX_EXIT_PF_GUEST_INS] = {
		.handler = mmio_access_vmexit_handler},
	[HX_EXIT_PF_GUEST_LOAD] = {
		.handler = mmio_access_vmexit_handler},
	[HX_EXIT_VIRT_INS] = {
		.handler = hlt_vmexit_handler},
	[HX_EXIT_PF_GUEST_STORE] = {
		.handler = mmio_access_vmexit_handler},
};
#endif

#define HX_VMEXIT_TYPE_MASK 0x8000000000000000
#define HX_VMEXIT_REASON_MASK 0xFFFFFFFF
int32_t vmexit_handler(struct acrn_vcpu *vcpu)
{
	struct vm_exit_dispatch *dispatch = NULL;
	uint16_t basic_exit_reason, exit_type;
	int32_t ret;
	const struct vm_exit_dispatch *dispatch_table;

	if (get_pcpu_id() != pcpuid_from_vcpu(vcpu)) {
		pr_fatal("vcpu is not running on its pcpu!");
		ret = -EINVAL;
	} else {
		/* Calculate basic exit reason (low 16-bits) */
		basic_exit_reason = (uint16_t)(vcpu->arch.exit_reason & HX_VMEXIT_REASON_MASK);
		exit_type = (uint16_t)((vcpu->arch.exit_reason & HX_VMEXIT_TYPE_MASK) != 0);

		/* Log details for exit */
//		pr_info("Exit Reason: 0x%016lx ", vcpu->arch.exit_reason);
		if (!exit_type)
			dispatch_table = exception_dispatch_table;
		else
			dispatch_table = interrupt_dispatch_table;

		/* Ensure exit reason is within dispatch table */
		if ((!exit_type && basic_exit_reason >= ARRAY_SIZE(exception_dispatch_table)) ||
		    (exit_type && basic_exit_reason >= ARRAY_SIZE(interrupt_dispatch_table)))
		{
			pr_info("Invalid Exit Reason: 0x%016lx ", vcpu->arch.exit_reason);
			ret = -EINVAL;
		} else {
			/* Calculate dispatch table entry */
			dispatch = (struct vm_exit_dispatch *)(dispatch_table + basic_exit_reason);
			/* See if an exit qualification is necessary for this exit handler */
			if (dispatch->need_exit_qualification != 0U) {
				/* Get exit qualification */
			} else {
				vcpu->arch.exit_qualification = basic_exit_reason;
			}

			ret = dispatch->handler(vcpu);
		}
	}

	return ret;
}
