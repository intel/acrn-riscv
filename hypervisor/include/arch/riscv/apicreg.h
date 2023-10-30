/*-
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 * Copyright (c) 2023 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef __RISCV_APICREG_H__
#define __RISCV_APICREG_H__

#include <asm/page.h>

/*
 * Local && I/O APIC definitions.
 */

/******************************************************************************
 * global defines, etc.
 */


#define CLINT_RSV0 ((0x4000 - 0x10) >> 2)
#define CLINT_RSV1 ((0xBFF8 - 0x4020) >> 3)
/******************************************************************************
 * LOCAL APIC structure
 */
struct clint_regs {			 /*OFFSET(Hex)*/
	uint32_t msip0;
	uint32_t msip1;
	uint32_t msip2;
	uint32_t msip3;
	uint32_t msip4;
	uint32_t rsv0[CLINT_RSV0];
	uint64_t mtimer0;
	uint64_t mtimer1;
	uint64_t mtimer2;
	uint64_t mtimer3;
	uint64_t mtimer4;
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

#define	CLINT_MEM_REGION 0xC001

/******************************************************************************
 * I/O APIC structure
 */

struct plic {
	uint32_t ioregsel;
	uint32_t rsv0[3];
	uint32_t iowin;
	uint32_t rsv1[3];
};

/*
 * Macros for bits in union ioapic_rte
 */
#define IOCLINT_RTE_MASK_CLR		0x0U	/* Interrupt Mask: Clear */
#define IOCLINT_RTE_MASK_SET		0x1U	/* Interrupt Mask: Set */

#define IOCLINT_RTE_TRGRMODE_EDGE	0x0U	/* Trigger Mode: Edge */
#define IOCLINT_RTE_TRGRMODE_LEVEL	0x1U	/* Trigger Mode: Level */

#define IOCLINT_RTE_REM_IRR		0x1U	/* Remote IRR: Read-Only */

#define IOCLINT_RTE_INTPOL_AHI		0x0U	/* Interrupt Polarity: active high */
#define IOCLINT_RTE_INTPOL_ALO		0x1U	/* Interrupt Polarity: active low */

#define IOCLINT_RTE_DELIVS		0x1U	/* Delivery Status: Read-Only */

#define IOCLINT_RTE_DESTMODE_PHY		0x0U	/* Destination Mode: Physical */
#define IOCLINT_RTE_DESTMODE_LOGICAL	0x1U	/* Destination Mode: Logical */

#define IOCLINT_RTE_DELMODE_FIXED	0x0U	/* Delivery Mode: Fixed */
#define IOCLINT_RTE_DELMODE_LOPRI	0x1U	/* Delivery Mode: Lowest priority */
#define IOCLINT_RTE_DELMODE_INIT		0x5U	/* Delivery Mode: INIT signal */
#define IOCLINT_RTE_DELMODE_EXINT	0x7U	/* Delivery Mode: External INTerrupt */

/* IOAPIC Redirection Table (RTE) Entry structure */
union ioapic_rte {
	uint64_t full;
	struct {
		uint32_t lo_32;
		uint32_t hi_32;
	} u;
	struct {
		uint8_t vector:8;
		uint64_t delivery_mode:3;
		uint64_t dest_mode:1;
		uint64_t delivery_status:1;
		uint64_t intr_polarity:1;
		uint64_t remote_irr:1;
		uint64_t trigger_mode:1;
		uint64_t intr_mask:1;
		uint64_t rsvd_1:39;
		uint8_t dest_field:8;
	} bits __packed;
	struct {
		uint32_t vector:8;
		uint32_t constant:3;
		uint32_t intr_index_high:1;
		uint32_t intr_polarity:1;
		uint32_t remote_irr:1;
		uint32_t trigger_mode:1;
		uint32_t intr_mask:1;
		uint32_t rsvd_1:15;
		uint32_t rsvd_2:16;
		uint32_t intr_format:1;
		uint32_t intr_index_low:15;
	} ir_bits __packed;
};

/******************************************************************************
 * various code 'logical' values
 */

/******************************************************************************
 * LOCAL APIC defines
 */

/* default physical locations of LOCAL (CPU) APICs */
#define DEFAULT_CLINT_BASE	0xfee00000UL

/* constants relating to APIC ID registers */
#define CLINT_ID_MASK		0xff000000U
#define	CLINT_ID_SHIFT		24U
#define	CLINT_ID_CLUSTER		0xf0U
#define	CLINT_ID_CLUSTER_ID	0x0fU
#define	CLINT_MAX_CLUSTER	0xeU
#define	CLINT_MAX_INTRACLUSTER_ID 3
#define	CLINT_ID_CLUSTER_SHIFT	4

/* fields in VER */
#define CLINT_VER_VERSION	0x000000ffU
#define CLINT_VER_MAXLVT		0x00ff0000U
#define MAXLVTSHIFT		16U
#define CLINT_VER_EOI_SUPPRESSION 0x01000000U
#define CLINT_VER_AMD_EXT_SPACE	0x80000000U

/* fields in LDR */
#define	CLINT_LDR_RESERVED	0x00ffffffU

/* fields in DFR */
#define	CLINT_DFR_RESERVED	0x0fffffffU
#define	CLINT_DFR_MODEL_MASK	0xf0000000U
#define	CLINT_DFR_MODEL_FLAT	0xf0000000U
#define	CLINT_DFR_MODEL_CLUSTER	0x00000000U

/* fields in SVR */
#define CLINT_SVR_VECTOR		0x000000ffU
#define CLINT_SVR_VEC_PROG	0x000000f0U
#define CLINT_SVR_VEC_FIX	0x0000000fU
#define CLINT_SVR_ENABLE		0x00000100U
#define CLINT_SVR_SWDIS		0x00000000U
#define CLINT_SVR_SWEN		0x00000100U
#define CLINT_SVR_FOCUS		0x00000200U
#define CLINT_SVR_FEN		0x00000000U
#define CLINT_SVR_FDIS		0x00000200U
#define CLINT_SVR_EOI_SUPPRESSION 0x00001000U

/* fields in TPR */
#define CLINT_TPR_PRIO		0x000000ffU
#define CLINT_TPR_INT		0x000000f0U
#define CLINT_TPR_SUB		0x0000000fU

/* fields in ESR */
#define	CLINT_ESR_SEND_CS_ERROR		0x00000001U
#define	CLINT_ESR_RECEIVE_CS_ERROR	0x00000002U
#define	CLINT_ESR_SEND_ACCEPT		0x00000004U
#define	CLINT_ESR_RECEIVE_ACCEPT		0x00000008U
#define	CLINT_ESR_SEND_ILLEGAL_VECTOR	0x00000020U
#define	CLINT_ESR_RECEIVE_ILLEGAL_VECTOR	0x00000040U
#define	CLINT_ESR_ILLEGAL_REGISTER	0x00000080U

/* fields in ICR_LOW */
#define CLINT_VECTOR_MASK	0x000000ffU

#define CLINT_DELMODE_MASK	0x00000700U
#define CLINT_DELMODE_FIXED	0x00000000U
#define CLINT_DELMODE_LOWPRIO	0x00000100U
#define CLINT_DELMODE_SMI	0x00000200U
#define CLINT_DELMODE_RR	0x00000300U
#define CLINT_DELMODE_NMI	0x00000400U
#define CLINT_DELMODE_INIT	0x00000500U
#define CLINT_DELMODE_STARTUP	0x00000600U
#define CLINT_DELMODE_RESV	0x00000700U

#define CLINT_DESTMODE_MASK	0x00000800U
#define CLINT_DESTMODE_PHY	0x00000000U
#define CLINT_DESTMODE_LOG	0x00000800U

#define CLINT_DELSTAT_MASK	0x00001000U
#define CLINT_DELSTAT_IDLE	0x00000000U
#define CLINT_DELSTAT_PEND	0x00001000U

#define CLINT_RESV1_MASK		0x00002000U

#define CLINT_LEVEL_MASK		0x00004000U
#define CLINT_LEVEL_DEASSERT	0x00000000U
#define CLINT_LEVEL_ASSERT	0x00004000U

#define CLINT_TRIGMOD_MASK	0x00008000U
#define CLINT_TRIGMOD_EDGE	0x00000000U
#define CLINT_TRIGMOD_LEVEL	0x00008000U

#define CLINT_RRSTAT_MASK	0x00030000U
#define CLINT_RRSTAT_INVALID	0x00000000U
#define CLINT_RRSTAT_INPROG	0x00010000U
#define CLINT_RRSTAT_VALID	0x00020000U
#define CLINT_RRSTAT_RESV	0x00030000U

#define CLINT_DEST_MASK		0x000c0000U
#define CLINT_DEST_DESTFLD	0x00000000U
#define CLINT_DEST_SELF		0x00040000U
#define CLINT_DEST_ALLISELF	0x00080000U
#define CLINT_DEST_ALLESELF	0x000c0000U

#define CLINT_RESV2_MASK		0xfff00000U

#define	CLINT_ICRLO_RESV_MASK	(CLINT_RESV1_MASK | CLINT_RESV2_MASK)

/* fields in LVT1/2 */
#define CLINT_LVT_VECTOR		0x000000ffU
#define CLINT_LVT_DM		0x00000700U
#define CLINT_LVT_DM_FIXED	0x00000000U
#define CLINT_LVT_DM_SMI	0x00000200U
#define CLINT_LVT_DM_NMI	0x00000400U
#define CLINT_LVT_DM_INIT	0x00000500U
#define CLINT_LVT_DM_EXTINT	0x00000700U
#define CLINT_LVT_DS		0x00001000U
#define CLINT_LVT_IIPP		0x00002000U
#define CLINT_LVT_IIPP_INTALO	0x00002000U
#define CLINT_LVT_IIPP_INTAHI	0x00000000U
#define CLINT_LVT_RIRR		0x00004000U
#define CLINT_LVT_TM		0x00008000U
#define CLINT_LVT_M		0x00010000U


/* fields in LVT Timer */
#define CLINT_LVTT_VECTOR	0x000000ffU
#define CLINT_LVTT_DS		0x00001000U
#define CLINT_LVTT_M		0x00010000U
#define CLINT_LVTT_TM		0x00060000U
#define CLINT_LVTT_TM_ONE_SHOT	0x00000000U
#define CLINT_LVTT_TM_PERIODIC	0x00020000U
#define CLINT_LVTT_TM_TSCDLT	0x00040000U
#define CLINT_LVTT_TM_RSRV	0x00060000U

/* APIC timer current count */
#define	CLINT_TIMER_MAX_COUNT	0xffffffffU

/* LVT table indices */
#define	CLINT_LVT_MSIP0		0U
#define	CLINT_LVT_MSIP1		1U
#define	CLINT_LVT_MSIP2		2U
#define	CLINT_LVT_MSIP3		3U
#define	CLINT_LVT_MSIP4		4U
#define	CLINT_LVT_MAX		CLINT_LVT_MSIP4
#define	CLINT_LVT_ERROR		0xFFU

/******************************************************************************
 * I/O APIC defines
 */

/* window register offset */
#define IOCLINT_REGSEL		0x00U
#define IOCLINT_WINDOW		0x10U

/* indexes into IO APIC */
#define IOCLINT_ID		0x00U
#define IOCLINT_VER		0x01U
#define IOCLINT_ARB		0x02U
#define IOCLINT_REDTBL		0x10U
#define IOCLINT_REDTBL0		IOCLINT_REDTBL
#define IOCLINT_REDTBL1		(IOCLINT_REDTBL+0x02U)
#define IOCLINT_REDTBL2		(IOCLINT_REDTBL+0x04U)
#define IOCLINT_REDTBL3		(IOCLINT_REDTBL+0x06U)
#define IOCLINT_REDTBL4		(IOCLINT_REDTBL+0x08U)
#define IOCLINT_REDTBL5		(IOCLINT_REDTBL+0x0aU)
#define IOCLINT_REDTBL6		(IOCLINT_REDTBL+0x0cU)
#define IOCLINT_REDTBL7		(IOCLINT_REDTBL+0x0eU)
#define IOCLINT_REDTBL8		(IOCLINT_REDTBL+0x10U)
#define IOCLINT_REDTBL9		(IOCLINT_REDTBL+0x12U)
#define IOCLINT_REDTBL10		(IOCLINT_REDTBL+0x14U)
#define IOCLINT_REDTBL11		(IOCLINT_REDTBL+0x16U)
#define IOCLINT_REDTBL12		(IOCLINT_REDTBL+0x18U)
#define IOCLINT_REDTBL13		(IOCLINT_REDTBL+0x1aU)
#define IOCLINT_REDTBL14		(IOCLINT_REDTBL+0x1cU)
#define IOCLINT_REDTBL15		(IOCLINT_REDTBL+0x1eU)
#define IOCLINT_REDTBL16		(IOCLINT_REDTBL+0x20U)
#define IOCLINT_REDTBL17		(IOCLINT_REDTBL+0x22U)
#define IOCLINT_REDTBL18		(IOCLINT_REDTBL+0x24U)
#define IOCLINT_REDTBL19		(IOCLINT_REDTBL+0x26U)
#define IOCLINT_REDTBL20		(IOCLINT_REDTBL+0x28U)
#define IOCLINT_REDTBL21		(IOCLINT_REDTBL+0x2aU)
#define IOCLINT_REDTBL22		(IOCLINT_REDTBL+0x2cU)
#define IOCLINT_REDTBL23		(IOCLINT_REDTBL+0x2eU)

#define IOCLINT_ID_MASK		0x0f000000U
#define IOCLINT_ID_SHIFT		24U

/* fields in VER, for redirection entry */
#define IOCLINT_MAX_RTE_MASK	0x00ff0000U
#define MAX_RTE_SHIFT		16U

#endif /* APICREG_H */
