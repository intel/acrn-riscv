/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_APICREG_H__
#define __RISCV_APICREG_H__

#include <asm/page.h>

#define CLINT_RSV0 ((0x4000 - 0x10) >> 2)
#define CLINT_RSV1 ((0xBFF8 - 0x4020) >> 3)

struct clint_regs {
	uint32_t msip[5];
	uint32_t rsv0[CLINT_RSV0];
	uint64_t mtimer[5];
	uint64_t rsv1[CLINT_RSV1];
	uint64_t mtime;
	uint64_t rsv2;
} __aligned(PAGE_SIZE);

enum CLINT_REGISTERS {
	MSIP0		= 0x0,
	MSIP1	 	= 0x4,
	MSIP2	 	= 0x8,
	MSIP3	 	= 0xC,
	MSIP4	 	= 0x10,
	MTIMER0 	= 0x4000,
	MTIMER1 	= 0x4008,
	MTIMER2 	= 0x4010,
	MTIMER3 	= 0x4018,
	MTIMER4 	= 0x4020,
	MTIME	 	= 0xBFF8,
};

#define	CLINT_MEM_ADDR		CONFIG_CLINT_BASE
#define	CLINT_MEM_REGION	CLINT_MEM_ADDR + CONFIG_CLINT_SIZE

#define DEFAULT_CLINT_BASE	CONFIG_CLINT_BASE
#define DEFAULT_CLINT_SIZE	CONFIG_CLINT_SIZE

/* LVT table indices */
#define	CLINT_LVT_MSIP0		0U
#define	CLINT_LVT_MSIP1		1U
#define	CLINT_LVT_MSIP2		2U
#define	CLINT_LVT_MSIP3		3U
#define	CLINT_LVT_MSIP4		4U
#define	CLINT_LVT_MAX		(CLINT_LVT_MSIP4 + 1)
#define	CLINT_LVT_ERROR		0xFFU

#define MAX_RTE_SHIFT		16U

#define CLINT_VECTOR_SSI       0x00000002U
#define CLINT_VECTOR_STI       0x00000020U
#define CLINT_VECTOR_SEI       0x00000200U

// PLIC definitions
#define PLIC_MEM_ADDR		CONFIG_PLIC_BASE
#define PLIC_MEM_REGION		PLIC_MEM_ADDR + CONFIG_PLIC_SIZE

#define DEFAULT_PLIC_BASE	0x0C000000UL
#define DEFAULT_PLIC_SIZE	0x04000000

#define PLIC_NUM_SOURCES	0x60
#define PLIC_NUM_PRIORITY	8
#define PLIC_NUM_CONTEXT	8
#define PLIC_NUM_FIELDS		((PLIC_NUM_SOURCES + 31)/32)

#define PLIC_SRC_PRIORITY_BASE	0x00
#define PLIC_PENDING_BASE	0x1000
#define PLIC_ENABLE_BASE	0x2000
#define PLIC_ENABLE_STRIDE	0x80
#define PLIC_DST_PRIO_BASE	0x200000
#define PLIC_DST_PRIO_STRIDE 	0x1000

struct plic_regs {
        uint32_t source_priority[PLIC_NUM_SOURCES];
        uint32_t pending[PLIC_NUM_FIELDS];
        uint32_t enable[PLIC_NUM_CONTEXT][PLIC_NUM_FIELDS];
        uint32_t target_priority[PLIC_NUM_CONTEXT];
        uint32_t claimed[PLIC_NUM_FIELDS];
} __aligned(PAGE_SIZE);

extern void init_clint_ipi(void);
#endif /* __RISCV_APICREG_H__ */
