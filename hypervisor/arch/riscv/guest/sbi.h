/*
 * Copyright (C) 2023-2024 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_GUEST_SBI_H__
#define __RISCV_GUEST_SBI_H__

#include <asm/cpu.h>
#include <asm/sbi.h>
#include <lib/types.h>

#define SBI_SPEC_VERSION_MAJOR			0x2
#define SBI_SPEC_VERSION_MINOR			0x0
#define SBI_ACRN_IMPID				0x1
#define SBI_ACRN_VERSION_MAJOR			0x0
#define SBI_ACRN_VERSION_MINOR			0x1

enum sbi_type {
	SBI_TYPE_BASE,
	SBI_TYPE_TIMER,
	SBI_TYPE_IPI,
	SBI_TYPE_RFENCE,
	SBI_TYPE_HSM,
	SBI_TYPE_SRST,
	SBI_TYPE_PMU,
	SBI_TYPE_MPXY,
	SBI_MAX_TYPES,
};

extern int sbi_ecall_handler(struct acrn_vcpu *vcpu);

struct sbi_ecall_dispatch {
	enum sbi_id ext_id;
	void (*handler)(struct acrn_vcpu *, struct cpu_regs *regs);
};

struct sbi_rfence_call {
	uint64_t base;
	uint64_t size;
	uint64_t asid;
};

#endif /* __RISCV_GUEST_SBI_H__ */
