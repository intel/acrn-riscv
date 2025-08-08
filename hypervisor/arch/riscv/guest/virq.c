/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <errno.h>
#include <asm/lib/bits.h>
#include <asm/irq.h>
#include <asm/vmx.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/vmcs.h>
#include <asm/guest/vm.h>
#include <asm/guest/virq.h>
#include <trace.h>
#include <logmsg.h>

#define EXCEPTION_ERROR_CODE_VALID  8U
#define DBG_LEVEL_INTR	6U

static const uint16_t exception_type[32] = {
	[0] = HX_INT_TYPE_HW_EXP,
	[1] = HX_INT_TYPE_HW_EXP,
	[2] = HX_INT_TYPE_HW_EXP,
	[3] = HX_INT_TYPE_HW_EXP,
	[4] = HX_INT_TYPE_HW_EXP,
	[5] = HX_INT_TYPE_HW_EXP,
	[6] = HX_INT_TYPE_HW_EXP,
	[7] = HX_INT_TYPE_HW_EXP,
	[8] = HX_INT_TYPE_HW_EXP | EXCEPTION_ERROR_CODE_VALID,
	[9] = HX_INT_TYPE_HW_EXP,
	[10] = HX_INT_TYPE_HW_EXP | EXCEPTION_ERROR_CODE_VALID,
	[11] = HX_INT_TYPE_HW_EXP | EXCEPTION_ERROR_CODE_VALID,
	[12] = HX_INT_TYPE_HW_EXP | EXCEPTION_ERROR_CODE_VALID,
	[13] = HX_INT_TYPE_HW_EXP | EXCEPTION_ERROR_CODE_VALID,
	[14] = HX_INT_TYPE_HW_EXP | EXCEPTION_ERROR_CODE_VALID,
	[15] = HX_INT_TYPE_HW_EXP,
	[16] = HX_INT_TYPE_HW_EXP,
	[17] = HX_INT_TYPE_HW_EXP | EXCEPTION_ERROR_CODE_VALID,
	[18] = HX_INT_TYPE_HW_EXP,
	[19] = HX_INT_TYPE_HW_EXP,
	[20] = HX_INT_TYPE_HW_EXP,
	[21] = HX_INT_TYPE_HW_EXP,
	[22] = HX_INT_TYPE_HW_EXP,
	[23] = HX_INT_TYPE_HW_EXP,
	[24] = HX_INT_TYPE_HW_EXP,
	[25] = HX_INT_TYPE_HW_EXP,
	[26] = HX_INT_TYPE_HW_EXP,
	[27] = HX_INT_TYPE_HW_EXP,
	[28] = HX_INT_TYPE_HW_EXP,
	[29] = HX_INT_TYPE_HW_EXP,
	[30] = HX_INT_TYPE_HW_EXP,
	[31] = HX_INT_TYPE_HW_EXP
};

static bool is_guest_irq_enabled(struct acrn_vcpu *vcpu)
{
	uint64_t ie = 0;
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	ie = ctx->sie & 0x222;
	pr_dbg("%s: ie 0x%lx", __func__, ie);

	return !!ie;
}

void vcpu_make_request(struct acrn_vcpu *vcpu, uint16_t eventid)
{
	bitmap_set_lock(eventid, &vcpu->arch.pending_req);
	kick_vcpu(vcpu);
}

#ifndef CONFIG_MACRN
#if 0
/*
 * @retval true when INT is injected to guest.
 * @retval false when otherwise
 */
#if 0
static bool vcpu_do_pending_extint(const struct acrn_vcpu *vcpu)
{
	struct acrn_vm *vm;
	struct acrn_vcpu *primary;
	uint32_t vector;
	bool ret = false;

	vm = vcpu->vm;

	/* check if there is valid interrupt from vPIC, if yes just inject it */
	/* PIC only connect with primary CPU */
	primary = vcpu_from_vid(vm, BSP_CPU_ID);
	if (vcpu == primary) {
		//vplic_pending_intr(vm_pic(vcpu->vm), &vector);

		/*
		 * AndreiW FIXME:
		 */
		vector = NR_MAX_VECTOR;

		if (vector <= NR_MAX_VECTOR) {
			dev_dbg(DBG_LEVEL_INTR, "VPIC: to inject PIC vector %d\n",
					vector & 0xFFU);
			cpu_csr_write(hvip, vector);
			//vplic_intr_accepted(vcpu->vm->vplic, vector);
			ret = true;
		}
	}

	return ret;
}
#endif
#endif
#endif

int32_t vcpu_queue_exception(struct acrn_vcpu *vcpu, uint32_t vector)
{
	int32_t ret = 0;

	if (vector >= 32U) {
		pr_err("invalid exception vector %d", vector);
		ret = -EINVAL;
	} else {
		vcpu_make_request(vcpu, ACRN_REQUEST_EXCP);
	}

	return ret;
}

/*
 * @pre vcpu->arch.exception_info.exception < 0x20U
 */
static bool vcpu_inject_exception(struct acrn_vcpu *vcpu)
{
	bool injected = false;

	if (bitmap_test_and_clear_lock(ACRN_REQUEST_EXCP, &vcpu->arch.pending_req)) {
		uint32_t vector = vcpu->arch.exception_info.exception;

		if ((exception_type[vector] & EXCEPTION_ERROR_CODE_VALID) != 0U) {
		}

		vcpu->arch.exception_info.exception = VECTOR_INVALID;

		/* retain ip for exception injection */
		vcpu_retain_ip(vcpu);

		injected = true;
	}

	return injected;
}

/* Inject NMI to guest */
void vcpu_inject_nmi(struct acrn_vcpu *vcpu)
{
	vcpu_make_request(vcpu, ACRN_REQUEST_NMI);
	signal_event(&vcpu->events[VCPU_EVENT_VIRTUAL_INTERRUPT]);
}

/* Inject general protection exception(#GP) to guest */
void vcpu_inject_gp(struct acrn_vcpu *vcpu, uint32_t err_code)
{
}

/* Inject page fault exception(#PF) to guest */
void vcpu_inject_pf(struct acrn_vcpu *vcpu, uint64_t addr, uint32_t err_code)
{
}

/* Inject invalid opcode exception(#UD) to guest */
void vcpu_inject_ud(struct acrn_vcpu *vcpu)
{
}

/* Inject stack fault exception(#SS) to guest */
void vcpu_inject_ss(struct acrn_vcpu *vcpu)
{
}

int32_t interrupt_window_vmexit_handler(struct acrn_vcpu *vcpu)
{
	/* Disable interrupt-window exiting first.
	 * acrn_handle_pending_request will continue handle for this vcpu
	 */
	vcpu_retain_ip(vcpu);

	return 0;
}

int32_t external_interrupt_vmexit_handler(struct acrn_vcpu *vcpu)
{
	uint32_t intr_info;
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;
	int32_t ret;

	intr_info = cpu_csr_read(scause);
	if (((intr_info & HX_INT_INFO_VALID) == 0U) ||
		(((intr_info & HX_INT_TYPE_MASK) >> 8U)
		!= HX_INT_TYPE_EXT_INT)) {
		pr_err("Invalid VM exit interrupt info:%x", intr_info);
		vcpu_retain_ip(vcpu);
		ret = -EINVAL;
	} else {
		dispatch_interrupt(&ctx->cpu_gp_regs.regs);
		vcpu_retain_ip(vcpu);

		//TRACE_2L(TRACE_VMEXIT_EXTERNAL_INTERRUPT, ctx.vector, 0UL);
		ret = 0;
	}

	return ret;
}

int32_t mexti_vmexit_handler(struct acrn_vcpu *vcpu)
{
	handle_mexti();

	return 0;
}

static inline void acrn_inject_pending_intr(struct acrn_vcpu *vcpu,
		uint64_t *pending_req_bits, bool injected);

int32_t acrn_handle_pending_request(struct acrn_vcpu *vcpu)
{
	bool injected = false;
	int32_t ret = 0;
	struct acrn_vcpu_arch *arch = &vcpu->arch;
	uint64_t *pending_req_bits = &arch->pending_req;

	/* make sure ACRN_REQUEST_INIT_VMCS handler as the first one */
	if (bitmap_test_and_clear_lock(ACRN_REQUEST_INIT_VMCS, pending_req_bits)) {
		init_vmcs(vcpu);
	}

	if (bitmap_test_and_clear_lock(ACRN_REQUEST_TRP_FAULT, pending_req_bits)) {
		pr_fatal("Tiple fault happen -> shutdown!");
		ret = -EFAULT;
	} else {
		if (bitmap_test_and_clear_lock(ACRN_REQUEST_WAIT_WBINVD, pending_req_bits)) {
			wait_event(&vcpu->events[VCPU_EVENT_SYNC_WBINVD]);
		}

		if (bitmap_test_and_clear_lock(ACRN_REQUEST_EPT_FLUSH, pending_req_bits)) {
			//invept(vcpu->vm->arch_vm.s2ptp);
		}

		if (bitmap_test_and_clear_lock(ACRN_REQUEST_VPID_FLUSH,	pending_req_bits)) {
			//flush_vpid_single(arch->vpid);
		}

		if (bitmap_test_and_clear_lock(ACRN_REQUEST_EOI_EXIT_BITMAP_UPDATE, pending_req_bits)) {
			vcpu_set_vmcs_eoi_exit(vcpu);
		}

		/*
		 * Inject pending exception prior pending interrupt to complete the previous instruction.
		 */
		injected = vcpu_inject_exception(vcpu);
		acrn_inject_pending_intr(vcpu, pending_req_bits, injected);
	}

	return ret;
}

static inline void acrn_inject_pending_intr(struct acrn_vcpu *vcpu,
		uint64_t *pending_req_bits, bool injected)
{
	bool ret = injected;
	bool guest_irq_enabled = is_guest_irq_enabled(vcpu);

	if (guest_irq_enabled && (!ret)) {
		/* Inject external interrupt first */
		if (bitmap_test_and_clear_lock(ACRN_REQUEST_EXTINT, pending_req_bits)) {
			/* has pending external interrupts */
			vcpu_inject_extint(vcpu);
		}
		if (bitmap_test_and_clear_lock(ACRN_REQUEST_EVENT, pending_req_bits))
			vcpu_inject_intr(vcpu);
	}
}

/*
 * @pre vcpu != NULL
 */
int32_t exception_vmexit_handler(struct acrn_vcpu *vcpu)
{
	uint32_t exception_vector = VECTOR_INVALID;
	int32_t status = 0;

	pr_dbg(" Handling guest exception");

	/* Handle all other exceptions */
	vcpu_retain_ip(vcpu);

	status = vcpu_queue_exception(vcpu, exception_vector);

	return status;
}

int32_t nmi_window_vmexit_handler(struct acrn_vcpu *vcpu)
{
	vcpu_retain_ip(vcpu);

	return 0;
}
