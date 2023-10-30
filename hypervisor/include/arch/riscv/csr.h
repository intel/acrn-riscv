/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#ifndef __RISCV_CSR_H__
#define __RISCV_CSR_H__

/* architectural (common) CSRs */

#define CSR_HSTATUS			0x600U
#define CSR_HDELEG			0x602U

#define CSR_TIME_STAMP_COUNTER		0x00000010U
#define CSR_PLATFORM_ID			0x00000017U
#define CSR_APIC_BASE			0x0000001BU
#define CSR_MTRR_CAP			0x000000FEU
#define CSR_ARCH_CAPABILITIES		0x0000010AU
#define CSR_TSC_DEADLINE		0x000006E0U
#define CSR_TSC_ADJUST			0x0000003BU
#define CSR_TSC_AUX			0xC0000103U

/* FEATURE CONTROL bits */
#define CSR_FEATURE_CONTROL_HX			(1U << 7U)

#ifndef ASSEMBLER
struct acrn_vcpu;

void init_csr_emulation(struct acrn_vcpu *vcpu);
uint32_t vcsr_get_guest_csr_index(uint32_t csr);

#endif /* ASSEMBLER */

/* MTRR memory type definitions */
#define MTRR_MEM_TYPE_UC			0x00UL	/* uncached */
#define MTRR_MEM_TYPE_WC			0x01UL	/* write combining */
#define MTRR_MEM_TYPE_WT			0x04UL	/* write through */
#define MTRR_MEM_TYPE_WP			0x05UL	/* write protected */
#define MTRR_MEM_TYPE_WB			0x06UL	/* writeback */

/* misc. MTRR flag definitions */
#define MTRR_ENABLE				0x800U	/* MTRR enable */
#define MTRR_FIX_ENABLE				0x400U	/* fixed range MTRR enable */
#define MTRR_VALID				0x800U	/* MTRR setting is valid */

/* SPEC & PRED bit */
#define SPEC_ENABLE_IBRS			(1U << 0U)
#define SPEC_ENABLE_STIBP			(1U << 1U)
#define PRED_SET_IBPB				(1U << 0U)

/* IA32 ARCH Capabilities bit */
#define ARCH_CAP_RDCL_NO			(1UL << 0U)
#define ARCH_CAP_IBRS_ALL			(1UL << 1U)
#define ARCH_CAP_RSBA			(1UL << 2U)
#define ARCH_CAP_SKIP_L1DFL_VMENTRY	(1UL << 3U)
#define ARCH_CAP_SSB_NO			(1UL << 4U)
#define ARCH_CAP_MDS_NO			(1UL << 5U)
#define ARCH_CAP_IF_PSCHANGE_MC_NO		(1UL << 6U)

/* Flush L1 D-cache */
#define L1D_FLUSH				(1UL << 0U)

#endif /* CSR_H */
