/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#include <types.h>
#include <errno.h>
#include <asm/pgtable.h>
#include <asm/csr.h>
//#include <cpuid.h>
#include <asm/guest/vcpu.h>
#include <asm/guest/vm.h>
#include <asm/vmx.h>
//#include <sgx.h>
//#include <guest_pm.h>
//#include <ucode.h>
//#include <rdt.h>
#include <trace.h>
#include <logmsg.h>

#define INTERCEPT_DISABLE		(0U)
#define INTERCEPT_READ			(1U << 0U)
#define INTERCEPT_WRITE			(1U << 1U)
#define INTERCEPT_READ_WRITE		(INTERCEPT_READ | INTERCEPT_WRITE)

static const uint32_t emulated_guest_csrs[NUM_GUEST_CSRS] = {
	/*
	 * CSRs that trusty may touch and need isolation between secure and normal world
	 * This may include CSR_STAR, CSR_LSTAR, CSR_FMASK,
	 * CSR_KERNEL_GS_BASE, CSR_SYSENTER_ESP, CSR_SYSENTER_CS, CSR_SYSENTER_EIP
	 *
	 * Number of entries: NUM_WORLD_CSRS
	 */
	CSR_TSC_ADJUST,

	/*
	 * CSRs don't need isolation between worlds
	 * Number of entries: NUM_COMMON_CSRS
	 */
	CSR_TSC_DEADLINE,
	CSR_TIME_STAMP_COUNTER,
	CSR_APIC_BASE,
};

#define NUM_MTRR_CSRS	13U
static const uint32_t mtrr_csrs[NUM_MTRR_CSRS] = {
	CSR_MTRR_CAP,
};

/* Following CSRs are intercepted, but it throws GPs for any guest accesses */
#define NUM_UNSUPPORTED_CSRS	111U
static const uint32_t unsupported_csrs[NUM_UNSUPPORTED_CSRS] = {
};

/* emulated_guest_csrs[] shares same indexes with array vcpu->arch->guest_csrs[] */
uint32_t vcsr_get_guest_csr_index(uint32_t csr)
{
	uint32_t index;

	for (index = 0U; index < NUM_GUEST_CSRS; index++) {
		if (emulated_guest_csrs[index] == csr) {
			break;
		}
	}

	if (index == NUM_GUEST_CSRS) {
		pr_err("%s, CSR %x is not defined in array emulated_guest_csrs[]", __func__, csr);
	}

	return index;
}

static void enable_csr_interception(uint8_t *bitmap, uint32_t csr_arg, uint32_t mode)
{
	uint32_t read_offset = 0U;
	uint32_t write_offset = 2048U;
	uint32_t csr = csr_arg;
	uint8_t csr_bit;
	uint32_t csr_index;

	if ((csr <= 0x1FFFU) || ((csr >= 0xc0000000U) && (csr <= 0xc0001fffU))) {
		if ((csr & 0xc0000000U) != 0U) {
			read_offset = read_offset + 1024U;
			write_offset = write_offset + 1024U;
		}

		csr &= 0x1FFFU;
		csr_bit = 1U << (csr & 0x7U);
		csr_index = csr >> 3U;

		if ((mode & INTERCEPT_READ) == INTERCEPT_READ) {
			bitmap[read_offset + csr_index] |= csr_bit;
		} else {
			bitmap[read_offset + csr_index] &= ~csr_bit;
		}

		if ((mode & INTERCEPT_WRITE) == INTERCEPT_WRITE) {
			bitmap[write_offset + csr_index] |= csr_bit;
		} else {
			bitmap[write_offset + csr_index] &= ~csr_bit;
		}
	} else {
		pr_err("%s, Invalid CSR: 0x%x", __func__, csr);
	}
}

/**
 * @pre vcpu != NULL
 */
static void init_csr_area(struct acrn_vcpu *vcpu)
{
	struct acrn_vm_config *cfg = get_vm_config(vcpu->vm->vm_id);
	uint16_t vcpu_clos = cfg->clos[vcpu->vcpu_id];

	vcpu->arch.csr_area.count = 0U;
/*
	vcpu->arch.csr_area.guest[CSR_AREA_TSC_AUX].csr_index = CSR_TSC_AUX;
	vcpu->arch.csr_area.guest[CSR_AREA_TSC_AUX].value = vcpu->vcpu_id;
	vcpu->arch.csr_area.host[CSR_AREA_TSC_AUX].csr_index = CSR_TSC_AUX;
	vcpu->arch.csr_area.host[CSR_AREA_TSC_AUX].value = pcpuid_from_vcpu(vcpu);
	vcpu->arch.csr_area.count++;
*/
}

/**
 * @pre vcpu != NULL
 */
void init_csr_emulation(struct acrn_vcpu *vcpu)
{
	uint8_t *csr_bitmap = vcpu->arch.csr_bitmap;
	uint32_t csr, i;
	uint64_t value64;

	for (i = 0U; i < NUM_GUEST_CSRS; i++) {
		enable_csr_interception(csr_bitmap, emulated_guest_csrs[i], INTERCEPT_READ_WRITE);
	}

	for (i = 0U; i < NUM_MTRR_CSRS; i++) {
		enable_csr_interception(csr_bitmap, mtrr_csrs[i], INTERCEPT_READ_WRITE);
	}

	for (i = 0U; i < NUM_UNSUPPORTED_CSRS; i++) {
		enable_csr_interception(csr_bitmap, unsupported_csrs[i], INTERCEPT_READ_WRITE);
	}

	/* don't need to intercept rdcsr for these CSRs */
	enable_csr_interception(csr_bitmap, CSR_TIME_STAMP_COUNTER, INTERCEPT_WRITE);

	/* Setup CSR bitmap - Intel SDM Vol3 24.6.9 */
	value64 = hva2hpa(vcpu->arch.csr_bitmap);
	exec_vmwrite64(HX_CSR_BITMAP_FULL, value64);
	pr_dbg("HX_CSR_BITMAP: 0x%016lx ", value64);
}

bool has_core_cap(uint32_t bit_mask)
{
	return true;
}

/**
 * @pre vcpu != NULL
 */
int32_t rdcsr_vmexit_handler(struct acrn_vcpu *vcpu)
{
	int32_t err = 0;
	uint32_t csr;
	uint64_t v = 0UL;

	/* Read the csr value */
	csr = (uint32_t)vcpu_get_gpreg(vcpu, CPU_REG_A0);

	/* Do the required processing for each csr case */
	switch (csr) {
	case CSR_TSC_DEADLINE:
	{
		v = vclint_get_tsc_deadline_csr(vcpu_vclint(vcpu));
		break;
	}
	case CSR_TSC_ADJUST:
	{
		v = vcpu_get_guest_csr(vcpu, CSR_TSC_ADJUST);
		break;
	}
	case CSR_MTRR_CAP:
	{
		if (!vm_hide_mtrr(vcpu->vm)) {
			v = read_vmtrr(vcpu, csr);
		} else {
			err = -EACCES;
		}
		break;
	}
	case CSR_APIC_BASE:
	{
		/* Read APIC base */
		v = vclint_get_clintbase(vcpu_vclint(vcpu));
		break;
	}
	default:
	{
		pr_warn("%s(): vm%d vcpu%d reading CSR %lx not supported",
			__func__, vcpu->vm->vm_id, vcpu->vcpu_id, csr);
		err = -EACCES;
		v = 0UL;
		break;
	}
	}

	/* Store the CSR contents in RAX and RDX */
	vcpu_set_gpreg(vcpu, CPU_REG_A0, v & 0xffffffffU);
	vcpu_set_gpreg(vcpu, CPU_REG_A2, v >> 32U);

//	TRACE_2L(TRACE_VMEXIT_RDCSR, csr, v);

	return err;
}

/*
 * If HX_TSC_OFFSET_FULL is 0, no need to trap the write of TSC_DEADLINE because there is
 * no offset between vTSC and pTSC, in this case, only write to vTSC_ADJUST is trapped.
 */
static void set_tsc_csr_interception(struct acrn_vcpu *vcpu, bool interception)
{
	uint8_t *csr_bitmap = vcpu->arch.csr_bitmap;
	bool is_intercepted =
		((csr_bitmap[CSR_TSC_DEADLINE >> 3U] & (1U << (CSR_TSC_DEADLINE & 0x7U))) != 0U);

	if (!interception && is_intercepted) {
		enable_csr_interception(csr_bitmap, CSR_TSC_DEADLINE, INTERCEPT_DISABLE);
		enable_csr_interception(csr_bitmap, CSR_TSC_ADJUST, INTERCEPT_WRITE);
		/* If the timer hasn't expired, sync virtual TSC_DEADLINE to physical TSC_DEADLINE, to make the guest read the same tsc_deadline
		 * as it writes. This may change when the timer actually trigger.
		 * If the timer has expired, write 0 to the virtual TSC_DEADLINE.
		 */
		if (cpu_csr_read(CSR_TSC_DEADLINE) != 0UL) {
			cpu_csr_write(CSR_TSC_DEADLINE, vcpu_get_guest_csr(vcpu, CSR_TSC_DEADLINE));
		} else {
			vcpu_set_guest_csr(vcpu, CSR_TSC_DEADLINE, 0UL);
		}
	} else if (interception && !is_intercepted) {
		enable_csr_interception(csr_bitmap, CSR_TSC_DEADLINE, INTERCEPT_READ_WRITE);
		enable_csr_interception(csr_bitmap, CSR_TSC_ADJUST, INTERCEPT_READ_WRITE);
		/* sync physical TSC_DEADLINE to virtual TSC_DEADLINE */
		vcpu_set_guest_csr(vcpu, CSR_TSC_DEADLINE, cpu_csr_read(CSR_TSC_DEADLINE));
	} else {
		/* Do nothing */
	}
}

/*
 * Intel SDM 17.17.3: If an execution of WRCSR to the
 * TIME_STAMP_COUNTER CSR adds (or subtracts) value X from the
 * TSC, the logical processor also adds (or subtracts) value X from
 * the TSC_ADJUST CSR.
 *
 * So, here we should update VMCS.OFFSET and vAdjust accordingly.
 *   - VMCS.OFFSET = vTSC - pTSC
 *   - vAdjust += VMCS.OFFSET's delta
 */

/**
 * @pre vcpu != NULL
 */
static void set_guest_tsc(struct acrn_vcpu *vcpu, uint64_t guest_tsc)
{
	uint64_t tsc_delta, tsc_offset_delta, tsc_adjust;

	tsc_delta = guest_tsc - rdtsc();

	/* the delta between new and existing TSC_OFFSET */
	tsc_offset_delta = tsc_delta - exec_vmread64(HX_TSC_OFFSET_FULL);

	/* apply this delta to TSC_ADJUST */
	tsc_adjust = vcpu_get_guest_csr(vcpu, CSR_TSC_ADJUST);
	vcpu_set_guest_csr(vcpu, CSR_TSC_ADJUST, tsc_adjust + tsc_offset_delta);

	/* write to VMCS because rdtsc and rdtscp are not intercepted */
	exec_vmwrite64(HX_TSC_OFFSET_FULL, tsc_delta);

	set_tsc_csr_interception(vcpu, tsc_delta != 0UL);
}

/*
 * The policy of vART is that software in native can run in VM too. And in native side,
 * the relationship between the ART hardware and TSC is:
 *
 *   pTSC = (pART * M) / N + pAdjust
 *
 * The vART solution is:
 *   - Present the ART capability to guest through CPUID leaf
 *     15H for M/N which identical to the physical values.
 *   - PT devices see the pART (vART = pART).
 *   - Guest expect: vTSC = vART * M / N + vAdjust.
 *   - VMCS.OFFSET = vTSC - pTSC = vAdjust - pAdjust.
 *
 * So to support vART, we should do the following:
 *   1. if vAdjust and vTSC are changed by guest, we should change
 *      VMCS.OFFSET accordingly.
 *   2. Make the assumption that the pAjust is never touched by ACRN.
 */

/*
 * Intel SDM 17.17.3: "If an execution of WRCSR to the TSC_ADJUST
 * CSR adds (or subtracts) value X from that CSR, the logical
 * processor also adds (or subtracts) value X from the TSC."
 *
 * So, here we should update VMCS.OFFSET and vAdjust accordingly.
 *   - VMCS.OFFSET += vAdjust's delta
 *   - vAdjust = new vAdjust set by guest
 */

/**
 * @pre vcpu != NULL
 */
static void set_guest_tsc_adjust(struct acrn_vcpu *vcpu, uint64_t tsc_adjust)
{
	uint64_t tsc_offset, tsc_adjust_delta;

	/* delta of the new and existing TSC_ADJUST */
	tsc_adjust_delta = tsc_adjust - vcpu_get_guest_csr(vcpu, CSR_TSC_ADJUST);

	/* apply this delta to existing TSC_OFFSET */
	tsc_offset = exec_vmread64(HX_TSC_OFFSET_FULL);
	exec_vmwrite64(HX_TSC_OFFSET_FULL, tsc_offset + tsc_adjust_delta);

	/* TSC_ADJUST is supposed to carry the value it's written to */
	vcpu_set_guest_csr(vcpu, CSR_TSC_ADJUST, tsc_adjust);

	set_tsc_csr_interception(vcpu, (tsc_offset + tsc_adjust_delta ) != 0UL);
}

void guest_cpuid(struct acrn_vcpu *vcpu, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
}

/**
 * @pre vcpu != NULL
 */
int32_t wrcsr_vmexit_handler(struct acrn_vcpu *vcpu)
{
	int32_t err = 0;
	uint32_t csr;
	uint64_t v;

	/* Read the CSR ID */
	csr = (uint32_t)vcpu_get_gpreg(vcpu, CPU_REG_A0);

	/* Get the CSR contents */
	v = (vcpu_get_gpreg(vcpu, CPU_REG_A1) << 32U) |
		vcpu_get_gpreg(vcpu, CPU_REG_A2);

	/* Do the required processing for each csr case */
	switch (csr) {
	case CSR_TSC_DEADLINE:
	{
		vclint_set_tsc_deadline_csr(vcpu_vclint(vcpu), v);
		break;
	}
	case CSR_TSC_ADJUST:
	{
		set_guest_tsc_adjust(vcpu, v);
		break;
	}
	case CSR_TIME_STAMP_COUNTER:
	{
		set_guest_tsc(vcpu, v);
		break;
	}
	default:
	{
		pr_warn("%s(): vm%d vcpu%d writing CSR %lx not supported",
			__func__, vcpu->vm->vm_id, vcpu->vcpu_id, csr);
		err = -EACCES;
		break;
	}
	}

//	TRACE_2L(TRACE_VMEXIT_WRCSR, csr, v);

	return err;
}
