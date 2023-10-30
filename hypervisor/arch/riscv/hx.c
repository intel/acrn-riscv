/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#include <types.h>
#include <asm/csr.h>
#include <acrn/percpu.h>
#include <asm/pgtable.h>
#include <asm/vmx.h>
#include <asm/cpu.h>
#include <asm/current.h>

DEFINE_PER_CPU(void *, vmxon_region);
/* Per cpu data to hold the vmxon_region for each pcpu.
 * It will be used again when we start a pcpu after the pcpu was down.
 * S3 enter/exit will use it.
 * Only run on current pcpu.
 */
void hx_on(void)
{
	uint64_t tmp64;

	/* Read Feature ControL CSR */
	tmp64 = cpu_csr_read(CSR_HSTATUS);

	/* Check if feature control is locked */
	if ((tmp64 & CSR_FEATURE_CONTROL_HX) == 0U) {
		/* Lock and enable HX support */
		tmp64 |= CSR_FEATURE_CONTROL_HX;
		cpu_csr_write(CSR_HSTATUS, tmp64);
	}
}

/**
 * only run on current pcpu
 */
void hx_off(void)
{
	uint64_t tmp64;

	/* Read Feature ControL CSR */
	tmp64 = cpu_csr_read(CSR_HSTATUS);

	/* Check if feature control is locked */
	if ((tmp64 & CSR_FEATURE_CONTROL_HX) == 1U) {
		/* Lock and enable HX support */
		tmp64 &= CSR_FEATURE_CONTROL_HX;
		cpu_csr_write(CSR_HSTATUS, tmp64);
	}
}

uint64_t exec_vmread64(uint32_t field_full)
{
	uint64_t value;

	asm volatile (
		"nop"
		: "=r" (value)
		: "r"(field_full)
		: );

	return value;
}

uint32_t exec_vmread32(uint32_t field)
{
	uint64_t value;

	value = exec_vmread64(field);

	return (uint32_t)value;
}

uint16_t exec_vmread16(uint32_t field)
{
        uint64_t value;

        value = exec_vmread64(field);

        return (uint16_t)value;
}

void exec_vmwrite64(uint32_t field_full, uint64_t value)
{
	asm volatile (
		"nop "
		: : "r" (value), "r"(field_full)
		: "cc");
}

void exec_vmwrite32(uint32_t field, uint32_t value)
{
	exec_vmwrite64(field, (uint64_t)value);
}

void exec_vmwrite16(uint32_t field, uint16_t value)
{
	exec_vmwrite64(field, (uint64_t)value);
}
