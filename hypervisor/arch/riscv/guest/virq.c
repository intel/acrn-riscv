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

#define EXCEPTION_CLASS_BENIGN	1
#define EXCEPTION_CLASS_CONT	2
#define EXCEPTION_CLASS_PF	3

/* Exception types */
#define EXCEPTION_FAULT		0U
#define EXCEPTION_TRAP		1U
#define EXCEPTION_ABORT		2U
#define EXCEPTION_INTERRUPT	3U

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

static uint8_t get_exception_type(uint32_t vector)
{
	uint8_t type;

	/* Treat #DB as trap until decide to support Debug Registers */
	if ((vector > 31U) || (vector == IDT_NMI)) {
		type = EXCEPTION_INTERRUPT;
	} else if ((vector == IDT_DB) || (vector == IDT_BP) || (vector ==  IDT_OF)) {
		type = EXCEPTION_TRAP;
	} else if ((vector == IDT_DF) || (vector == IDT_MC)) {
		type = EXCEPTION_ABORT;
	} else {
		type = EXCEPTION_FAULT;
	}

	return type;
}

static bool is_guest_irq_enabled(struct acrn_vcpu *vcpu)
{
	uint64_t ie = 0;
	struct run_context *ctx =
		&vcpu->arch.contexts[vcpu->arch.cur_context].run_ctx;

	ie = ctx->sie & 0x222;
	pr_dbg("%s: ie 0x%lx", __func__, ie);

	return !!ie;
}

static inline bool is_nmi_injectable(void)
{
	uint64_t guest_state;

	guest_state = cpu_csr_read(vsstatus);

	return ((guest_state & (HV_ARCH_VCPU_BLOCKED_BY_STI |
		HV_ARCH_VCPU_BLOCKED_BY_MOVSS | HV_ARCH_VCPU_BLOCKED_BY_NMI)) == 0UL);
}

void vcpu_make_request(struct acrn_vcpu *vcpu, uint16_t eventid)
{
	bitmap_set_lock(eventid, &vcpu->arch.pending_req);
	kick_vcpu(vcpu);
}

#ifndef CONFIG_MACRN
/*
 * @retval true when INT is injected to guest.
 * @retval false when otherwise
 */
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

/* SDM Vol3 -6.15, Table 6-4 - interrupt and exception classes */
static int32_t get_excep_class(uint32_t vector)
{
	int32_t ret;

	if ((vector == IDT_DE) || (vector == IDT_TS) || (vector == IDT_NP) ||
		(vector == IDT_SS) || (vector == IDT_GP)) {
		ret = EXCEPTION_CLASS_CONT;
	} else if ((vector == IDT_PF) || (vector == IDT_VE)) {
		ret = EXCEPTION_CLASS_PF;
	} else {
		ret = EXCEPTION_CLASS_BENIGN;
	}

	return ret;
}

/**
 * @brief Check if the NMI is for notification purpose
 *
 * @return true, if the NMI is triggered for notifying vCPU
 * @return false, if the NMI is triggered for other purpose
 */
static bool is_notification_nmi(const struct acrn_vm *vm)
{
	return false;
}

int32_t vcpu_queue_exception(struct acrn_vcpu *vcpu, uint32_t vector_arg, uint32_t err_code_arg)
{
	struct acrn_vcpu_arch *arch = &vcpu->arch;
	uint32_t vector = vector_arg;
	uint32_t err_code = err_code_arg;
	int32_t ret = 0;

	/* VECTOR_INVALID is also greater than 32 */
	if (vector >= 32U) {
		pr_err("invalid exception vector %d", vector);
		ret = -EINVAL;
	} else {

		uint32_t prev_vector = arch->exception_info.exception;
		int32_t new_class, prev_class;

		/* SDM vol3 - 6.15, Table 6-5 - conditions for generating a
		 * double fault */
		prev_class = get_excep_class(prev_vector);
		new_class = get_excep_class(vector);
		if ((prev_vector == IDT_DF) && (new_class != EXCEPTION_CLASS_BENIGN)) {
			/* tiple fault happen - shutdwon mode */
			vcpu_make_request(vcpu, ACRN_REQUEST_TRP_FAULT);
		} else {
			if (((prev_class == EXCEPTION_CLASS_CONT) && (new_class == EXCEPTION_CLASS_CONT)) ||
				((prev_class == EXCEPTION_CLASS_PF) && (new_class != EXCEPTION_CLASS_BENIGN))) {
				/* generate double fault */
				vector = IDT_DF;
				err_code = 0U;
			} else {
				/* Trigger the given exception instead of override it with
				 * double/tiple fault. */
			}

			arch->exception_info.exception = vector;

			if ((exception_type[vector] & EXCEPTION_ERROR_CODE_VALID) != 0U) {
				arch->exception_info.error = err_code;
			} else {
				arch->exception_info.error = 0U;
			}

			if ((vector == IDT_NMI) && is_notification_nmi(vcpu->vm)) {
				/*
				 * Currently, ACRN doesn't support vNMI well and there is no well-designed
				 * way to check if the NMI is for notification or not. Here we take all the
				 * NMIs as notification NMI for lapic-pt VMs temporarily.
				 *
				 * TODO: Add a way in is_notification_nmi to check the NMI is for notification
				 *       or not in order to support vNMI.
				 */
				pr_dbg("This NMI is used as notification signal. So ignore it.");
			} else {
				vcpu_make_request(vcpu, ACRN_REQUEST_EXCP);
			}
		}
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

		/* SDM 17.3.1.1 For any fault-class exception except a debug exception generated in response to an
		 * instruction breakpoint, the value pushed for RF is 1.
		 * #DB is treated as Trap in get_exception_type, so RF will not be set for instruction breakpoint.
		 */
		if (get_exception_type(vector) == EXCEPTION_FAULT) {
			vcpu_set_status(vcpu, vcpu_get_status(vcpu) | HV_ARCH_VCPU_RFLAGS_RF);
		}

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
	(void)vcpu_queue_exception(vcpu, IDT_GP, err_code);
}

/* Inject page fault exception(#PF) to guest */
void vcpu_inject_pf(struct acrn_vcpu *vcpu, uint64_t addr, uint32_t err_code)
{
//	vcpu_set_cr2(vcpu, addr);
	(void)vcpu_queue_exception(vcpu, IDT_PF, err_code);
}

/* Inject invalid opcode exception(#UD) to guest */
void vcpu_inject_ud(struct acrn_vcpu *vcpu)
{
	(void)vcpu_queue_exception(vcpu, IDT_UD, 0);
}

/* Inject stack fault exception(#SS) to guest */
void vcpu_inject_ss(struct acrn_vcpu *vcpu)
{
	(void)vcpu_queue_exception(vcpu, IDT_SS, 0);
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
	uint32_t int_err_code = 0U;
	uint32_t exception_vector = VECTOR_INVALID;
	int32_t status = 0;

	pr_dbg(" Handling guest exception");

	/* Handle all other exceptions */
	vcpu_retain_ip(vcpu);

	status = vcpu_queue_exception(vcpu, exception_vector, int_err_code);

	if (exception_vector == IDT_MC) {
		/* just print error message for #MC, it then will be injected
		 * back to guest */
		pr_fatal("Exception #MC got from guest!");
	}

	//TRACE_4I(TRACE_VMEXIT_EXCEPTION_OR_NMI,
	//		exception_vector, int_err_code, 2U, 0U);

	return status;
}

int32_t nmi_window_vmexit_handler(struct acrn_vcpu *vcpu)
{
	vcpu_retain_ip(vcpu);

	return 0;
}
