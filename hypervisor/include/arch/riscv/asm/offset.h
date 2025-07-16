/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_OFFSET_H__
#define __RISCV_OFFSET_H__

#define REG_EPC		0x0
#define REG_IP		0x0
#define REG_RA		0x8
#define REG_SP		0x10
#define REG_GP		0x18
#define REG_TP		0x20
#define REG_T0		0x28
#define REG_T1		0x30
#define REG_T2		0x38
#define REG_S0		0x40
#define REG_S1		0x48
#define REG_A0		0x50
#define REG_A1		0x58
#define REG_A2		0x60
#define REG_A3		0x68
#define REG_A4		0x70
#define REG_A5		0x78
#define REG_A6		0x80
#define REG_A7		0x88
#define REG_S2		0x90
#define REG_S3		0x98
#define REG_S4		0xA0
#define REG_S5		0xA8
#define REG_S6		0xB0
#define REG_S7		0xB8
#define REG_S8		0xC0
#define REG_S9		0xC8
#define REG_S10		0xD0
#define REG_S11		0xD8
#define REG_T3		0xE0
#define REG_T4		0xE8
#define REG_T5		0xF0
#define REG_T6		0xF8
#define REG_STATUS	0x100
#define REG_TVAL	0x108
#define REG_CAUSE	0x110
#define REG_HSTATUS	0x118
#define REG_HTVAL	0x120
#define REG_HTINST	0x128
#define REG_ORIG_A0	0x130

#ifdef CONFIG_MACRN
#define CSR_TVEC mtvec
#define CSR_IE mie
#define CSR_STATUS mstatus
#else
#define CSR_TVEC stvec
#define CSR_IE sie
#define CSR_STATUS sstatus
#endif

#endif /* __RISCV_OFFSET_H__ */
