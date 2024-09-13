/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_VCSR_H__
#define __RISCV_VCSR_H__

#define CSR_HSTATUS			0x600U
#define CSR_HDELEG			0x602U

#define CSR_PLATFORM_ID			0x00000017U
#define CSR_APIC_BASE			0x0000001BU
#define CSR_MTRR_CAP			0x000000FEU
#define CSR_ARCH_CAPABILITIES		0x0000010AU
#define CSR_TSC_DEADLINE		0x000006E0U
#define CSR_TSC_AUX			0xC0000103U

#define CSR_FEATURE_CONTROL_HX		(1U << 7U)

#ifndef __ASSEMBLY__
struct acrn_vcpu;

extern void init_csr_emulation(struct acrn_vcpu *vcpu);
extern uint32_t vcsr_get_guest_csr_index(uint32_t csr);
extern void guest_cpuid(struct acrn_vcpu *vcpu, uint32_t *eax, uint32_t *ebx,
			uint32_t *ecx, uint32_t *edx);
#endif /* __ASSEMBLY__ */

/* MTRR memory type definitions */
#define MTRR_MEM_TYPE_UC			0x00UL	/* uncached */
#define MTRR_MEM_TYPE_WC			0x01UL	/* write combining */
#define MTRR_MEM_TYPE_WT			0x04UL	/* write through */
#define MTRR_MEM_TYPE_WP			0x05UL	/* write protected */
#define MTRR_MEM_TYPE_WB			0x06UL	/* writeback */

/* Flush L1 D-cache */
#define L1D_FLUSH				(1UL << 0U)

#endif /* __RISCV_VCSR_H__ */
