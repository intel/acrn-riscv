/*
 * Copyright (C) 2023-2024 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_SBI_H__
#define __RISCV_SBI_H__

#include <asm/cpu.h>
#include <lib/types.h>

enum sbi_id {
	SBI_ID_BASE = 0x10,
	SBI_ID_TIMER = 0x54494D45,
	SBI_ID_IPI = 0x735049,
	SBI_ID_RFENCE = 0x52464E43,
	SBI_ID_HSM = 0x48534D,
	SBI_ID_SRST = 0x53525354,
	SBI_ID_PMU = 0x504D55,
	SBI_ID_MPXY = 0x4D505859,

	/* Experimentals extensions must lie within this range */
	SBI_EXPERIMENTAL_START = 0x08000000,
	SBI_EXPERIMENTAL_END = 0x08FFFFFF,

	/* Vendor extensions must lie within this range */
	SBI_VENDOR_START = 0x09000000,
	SBI_VENDOR_END = 0x09FFFFFF,
};

#define SBI_SPEC_VERSION_MAJOR			0x2
#define SBI_SPEC_VERSION_MINOR			0x0
#define SBI_ACRN_IMPID				0x1
#define SBI_ACRN_VERSION_MAJOR			0x0
#define SBI_ACRN_VERSION_MINOR			0x1

#define SBI_TYPE_BASE_GET_SPEC_VERSION		0x0
#define SBI_TYPE_BASE_GET_IMP_ID		0x1
#define SBI_TYPE_BASE_GET_IMP_VERSION		0x2
#define SBI_TYPE_BASE_PROBE_EXT			0x3
#define SBI_TYPE_BASE_GET_MVENDORID		0x4
#define SBI_TYPE_BASE_GET_MARCHID		0x5
#define SBI_TYPE_BASE_GET_MIMPID		0x6

/* SBI function IDs for TIME extension*/
#define SBI_TYPE_TIME_SET_TIMER			0x0

/* SBI function IDs for IPI extension*/
#define SBI_TYPE_IPI_SEND_IPI			0x0

/* SBI function IDs for RFENCE extension*/
#define SBI_TYPE_RFENCE_FNECE_I			0x0
#define SBI_TYPE_RFENCE_SFNECE_VMA		0x1
#define SBI_TYPE_RFENCE_SFNECE_VMA_ASID		0x2

/* SBI function IDs for MPXY extension*/
#define SBI_TYPE_MPXY_GET_SHM_SIZE		0x0
#define SBI_TYPE_MPXY_SET_SHM			0x1
#define SBI_TYPE_MPXY_GET_CHANNEL_IDS		0x2
#define SBI_TYPE_MPXY_READ_ATTRS		0x3
#define SBI_TYPE_MPXY_WRITE_ATTRS		0x4
#define SBI_TYPE_MPXY_SEND_MSG_WITH_RESP	0x5
#define SBI_TYPE_MPXY_SEND_MSG_WITHOUT_RESP	0x6
#define SBI_TYPE_MPXY_GET_NOTFICATION_EVENTS	0x7

/* SBI return error codes */
#define SBI_SUCCESS				0
#define SBI_EFAILURE				-1
#define SBI_ENOTSUPP				-2
#define SBI_EINVAL_PARAM			-3
#define SBI_EDENIED				-4
#define SBI_EINVAL_ADDR				-5
#define SBI_EAVAILABLE				-6
#define SBI_ESTARTED				-7
#define SBI_ESTOPPED				-8

extern int sbi_ecall_handler(struct acrn_vcpu *vcpu);

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

struct sbi_ecall_dispatch {
	enum sbi_id ext_id;
	void (*handler)(struct acrn_vcpu *, struct cpu_regs *regs);
};

struct sbi_rfence_call {
	uint64_t base;
	uint64_t size;
	uint64_t asid;
};
#define SBI_RFENCE_FLUSH_ALL ((uint64_t)-1)

#endif /* __RISCV_SBI_H__ */
