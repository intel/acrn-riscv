/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_PLICREG_H__
#define __RISCV_PLICREG_H__

#include <asm/page.h>

struct plic {
	uint32_t ioregsel;
	uint32_t rsv0[3];
	uint32_t iowin;
	uint32_t rsv1[3];
};

/*
 * Macros for bits in union ioapic_rte
 */
#define IOAPIC_RTE_MASK_CLR		0x0U	/* Interrupt Mask: Clear */
#define IOAPIC_RTE_MASK_SET		0x1U	/* Interrupt Mask: Set */

#define IOAPIC_RTE_TRGRMODE_EDGE	0x0U	/* Trigger Mode: Edge */
#define IOAPIC_RTE_TRGRMODE_LEVEL	0x1U	/* Trigger Mode: Level */

#define IOAPIC_RTE_REM_IRR		0x1U	/* Remote IRR: Read-Only */

#define IOAPIC_RTE_INTPOL_AHI		0x0U	/* Interrupt Polarity: active high */
#define IOAPIC_RTE_INTPOL_ALO		0x1U	/* Interrupt Polarity: active low */

#define IOAPIC_RTE_DELIVS		0x1U	/* Delivery Status: Read-Only */

#define IOAPIC_RTE_DESTMODE_PHY		0x0U	/* Destination Mode: Physical */
#define IOAPIC_RTE_DESTMODE_LOGICAL	0x1U	/* Destination Mode: Logical */

#define IOAPIC_RTE_DELMODE_FIXED	0x0U	/* Delivery Mode: Fixed */
#define IOAPIC_RTE_DELMODE_LOPRI	0x1U	/* Delivery Mode: Lowest priority */
#define IOAPIC_RTE_DELMODE_INIT		0x5U	/* Delivery Mode: INIT signal */
#define IOAPIC_RTE_DELMODE_EXINT	0x7U	/* Delivery Mode: External INTerrupt */

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
#define DEFAULT_APIC_BASE	0xfee00000UL

/* constants relating to APIC ID registers */
#define APIC_ID_MASK		0xff000000U
#define	APIC_ID_SHIFT		24U
#define	APIC_ID_CLUSTER		0xf0U
#define	APIC_ID_CLUSTER_ID	0x0fU
#define	APIC_MAX_CLUSTER	0xeU
#define	APIC_MAX_INTRACLUSTER_ID 3
#define	APIC_ID_CLUSTER_SHIFT	4

/* fields in VER */
#define APIC_VER_VERSION	0x000000ffU
#define APIC_VER_MAXLVT		0x00ff0000U
#define MAXLVTSHIFT		16U
#define APIC_VER_EOI_SUPPRESSION 0x01000000U
#define APIC_VER_AMD_EXT_SPACE	0x80000000U

/* fields in LDR */
#define	APIC_LDR_RESERVED	0x00ffffffU

/* fields in DFR */
#define	APIC_DFR_RESERVED	0x0fffffffU
#define	APIC_DFR_MODEL_MASK	0xf0000000U
#define	APIC_DFR_MODEL_FLAT	0xf0000000U
#define	APIC_DFR_MODEL_CLUSTER	0x00000000U

/* fields in SVR */
#define APIC_SVR_VECTOR		0x000000ffU
#define APIC_SVR_VEC_PROG	0x000000f0U
#define APIC_SVR_VEC_FIX	0x0000000fU
#define APIC_SVR_ENABLE		0x00000100U
#define APIC_SVR_SWDIS		0x00000000U
#define APIC_SVR_SWEN		0x00000100U
#define APIC_SVR_FOCUS		0x00000200U
#define APIC_SVR_FEN		0x00000000U
#define APIC_SVR_FDIS		0x00000200U
#define APIC_SVR_EOI_SUPPRESSION 0x00001000U

/* fields in TPR */
#define APIC_TPR_PRIO		0x000000ffU
#define APIC_TPR_INT		0x000000f0U
#define APIC_TPR_SUB		0x0000000fU

/* fields in ESR */
#define	APIC_ESR_SEND_CS_ERROR		0x00000001U
#define	APIC_ESR_RECEIVE_CS_ERROR	0x00000002U
#define	APIC_ESR_SEND_ACCEPT		0x00000004U
#define	APIC_ESR_RECEIVE_ACCEPT		0x00000008U
#define	APIC_ESR_SEND_ILLEGAL_VECTOR	0x00000020U
#define	APIC_ESR_RECEIVE_ILLEGAL_VECTOR	0x00000040U
#define	APIC_ESR_ILLEGAL_REGISTER	0x00000080U

/* fields in ICR_LOW */
#define APIC_VECTOR_MASK	0x000000ffU

#define APIC_DELMODE_MASK	0x00000700U
#define APIC_DELMODE_FIXED	0x00000000U
#define APIC_DELMODE_LOWPRIO	0x00000100U
#define APIC_DELMODE_SMI	0x00000200U
#define APIC_DELMODE_RR	0x00000300U
#define APIC_DELMODE_NMI	0x00000400U
#define APIC_DELMODE_INIT	0x00000500U
#define APIC_DELMODE_STARTUP	0x00000600U
#define APIC_DELMODE_RESV	0x00000700U

#define APIC_DESTMODE_MASK	0x00000800U
#define APIC_DESTMODE_PHY	0x00000000U
#define APIC_DESTMODE_LOG	0x00000800U

#define APIC_DELSTAT_MASK	0x00001000U
#define APIC_DELSTAT_IDLE	0x00000000U
#define APIC_DELSTAT_PEND	0x00001000U

#define APIC_RESV1_MASK		0x00002000U

#define APIC_LEVEL_MASK		0x00004000U
#define APIC_LEVEL_DEASSERT	0x00000000U
#define APIC_LEVEL_ASSERT	0x00004000U

#define APIC_TRIGMOD_MASK	0x00008000U
#define APIC_TRIGMOD_EDGE	0x00000000U
#define APIC_TRIGMOD_LEVEL	0x00008000U

#define APIC_RRSTAT_MASK	0x00030000U
#define APIC_RRSTAT_INVALID	0x00000000U
#define APIC_RRSTAT_INPROG	0x00010000U
#define APIC_RRSTAT_VALID	0x00020000U
#define APIC_RRSTAT_RESV	0x00030000U

#define APIC_DEST_MASK		0x000c0000U
#define APIC_DEST_DESTFLD	0x00000000U
#define APIC_DEST_SELF		0x00040000U
#define APIC_DEST_ALLISELF	0x00080000U
#define APIC_DEST_ALLESELF	0x000c0000U

#define APIC_RESV2_MASK		0xfff00000U

#define	APIC_ICRLO_RESV_MASK	(APIC_RESV1_MASK | APIC_RESV2_MASK)

/* fields in LVT1/2 */
#define APIC_LVT_VECTOR		0x000000ffU
#define APIC_LVT_DM		0x00000700U
#define APIC_LVT_DM_FIXED	0x00000000U
#define APIC_LVT_DM_SMI	0x00000200U
#define APIC_LVT_DM_NMI	0x00000400U
#define APIC_LVT_DM_INIT	0x00000500U
#define APIC_LVT_DM_EXTINT	0x00000700U
#define APIC_LVT_DS		0x00001000U
#define APIC_LVT_IIPP		0x00002000U
#define APIC_LVT_IIPP_INTALO	0x00002000U
#define APIC_LVT_IIPP_INTAHI	0x00000000U
#define APIC_LVT_RIRR		0x00004000U
#define APIC_LVT_TM		0x00008000U
#define APIC_LVT_M		0x00010000U


/* fields in LVT Timer */
#define APIC_LVTT_VECTOR	0x000000ffU
#define APIC_LVTT_DS		0x00001000U
#define APIC_LVTT_M		0x00010000U
#define APIC_LVTT_TM		0x00060000U
#define APIC_LVTT_TM_ONE_SHOT	0x00000000U
#define APIC_LVTT_TM_PERIODIC	0x00020000U
#define APIC_LVTT_TM_TSCDLT	0x00040000U
#define APIC_LVTT_TM_RSRV	0x00060000U

/* APIC timer current count */
#define	APIC_TIMER_MAX_COUNT	0xffffffffU

/* fields in TDCR */
#define APIC_TDCR_2		0x00U
#define APIC_TDCR_4		0x01U
#define APIC_TDCR_8		0x02U
#define APIC_TDCR_16		0x03U
#define APIC_TDCR_32		0x08U
#define APIC_TDCR_64		0x09U
#define APIC_TDCR_128		0x0aU
#define APIC_TDCR_1		0x0bU

/******************************************************************************
 * I/O APIC defines
 */

/* window register offset */
#define IOAPIC_REGSEL		0x00U
#define IOAPIC_WINDOW		0x10U

/* indexes into IO APIC */
#define IOAPIC_ID		0x00U
#define IOAPIC_VER		0x01U
#define IOAPIC_ARB		0x02U
#define IOAPIC_REDTBL		0x10U
#define IOAPIC_REDTBL0		IOAPIC_REDTBL
#define IOAPIC_REDTBL1		(IOAPIC_REDTBL+0x02U)
#define IOAPIC_REDTBL2		(IOAPIC_REDTBL+0x04U)
#define IOAPIC_REDTBL3		(IOAPIC_REDTBL+0x06U)
#define IOAPIC_REDTBL4		(IOAPIC_REDTBL+0x08U)
#define IOAPIC_REDTBL5		(IOAPIC_REDTBL+0x0aU)
#define IOAPIC_REDTBL6		(IOAPIC_REDTBL+0x0cU)
#define IOAPIC_REDTBL7		(IOAPIC_REDTBL+0x0eU)
#define IOAPIC_REDTBL8		(IOAPIC_REDTBL+0x10U)
#define IOAPIC_REDTBL9		(IOAPIC_REDTBL+0x12U)
#define IOAPIC_REDTBL10		(IOAPIC_REDTBL+0x14U)
#define IOAPIC_REDTBL11		(IOAPIC_REDTBL+0x16U)
#define IOAPIC_REDTBL12		(IOAPIC_REDTBL+0x18U)
#define IOAPIC_REDTBL13		(IOAPIC_REDTBL+0x1aU)
#define IOAPIC_REDTBL14		(IOAPIC_REDTBL+0x1cU)
#define IOAPIC_REDTBL15		(IOAPIC_REDTBL+0x1eU)
#define IOAPIC_REDTBL16		(IOAPIC_REDTBL+0x20U)
#define IOAPIC_REDTBL17		(IOAPIC_REDTBL+0x22U)
#define IOAPIC_REDTBL18		(IOAPIC_REDTBL+0x24U)
#define IOAPIC_REDTBL19		(IOAPIC_REDTBL+0x26U)
#define IOAPIC_REDTBL20		(IOAPIC_REDTBL+0x28U)
#define IOAPIC_REDTBL21		(IOAPIC_REDTBL+0x2aU)
#define IOAPIC_REDTBL22		(IOAPIC_REDTBL+0x2cU)
#define IOAPIC_REDTBL23		(IOAPIC_REDTBL+0x2eU)

#define IOAPIC_ID_MASK		0x0f000000U
#define IOAPIC_ID_SHIFT		24U

/* fields in VER, for redirection entry */
#define IOAPIC_MAX_RTE_MASK	0x00ff0000U
#define MAX_RTE_SHIFT		16U

#endif /* __RISCV_PLICREG_H__ */
