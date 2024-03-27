/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <errno.h>
#include <asm/pgtable.h>
#include <asm/guest/vcsr.h>
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
	 * CSRs don't need isolation between worlds
	 * Number of entries: NUM_COMMON_CSRS
	 */
	CSR_TSC_DEADLINE,
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
void init_csr_emulation(struct acrn_vcpu *vcpu)
{
	uint8_t *csr_bitmap = vcpu->arch.csr_bitmap;
	uint32_t i;
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

	/* Setup CSR bitmap - Intel SDM Vol3 24.6.9 */
	value64 = hva2hpa(vcpu->arch.csr_bitmap);
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
		vclint_set_tsc_deadline_csr(vcpu_vclint(vcpu), 0, v);
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
