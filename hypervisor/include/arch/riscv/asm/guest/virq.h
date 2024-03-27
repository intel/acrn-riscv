/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_VIRQ_H__
#define __RISCV_VIRQ_H__

extern void vcpu_make_request(struct acrn_vcpu *vcpu, uint16_t eventid);
extern void vcpu_inject_extint(struct acrn_vcpu *vcpu);
extern void vcpu_inject_nmi(struct acrn_vcpu *vcpu);
extern void vcpu_inject_gp(struct acrn_vcpu *vcpu, uint32_t err_code);
extern void vcpu_inject_pf(struct acrn_vcpu *vcpu, uint64_t addr, uint32_t err_code);
extern void vcpu_inject_ud(struct acrn_vcpu *vcpu);
extern void vcpu_inject_ss(struct acrn_vcpu *vcpu);
extern int32_t interrupt_window_vmexit_handler(struct acrn_vcpu *vcpu);
extern int32_t external_interrupt_vmexit_handler(struct acrn_vcpu *vcpu);
extern int32_t acrn_handle_pending_request(struct acrn_vcpu *vcpu);
extern int32_t exception_vmexit_handler(struct acrn_vcpu *vcpu);
extern int32_t nmi_window_vmexit_handler(struct acrn_vcpu *vcpu);
void vcpu_inject_intr(struct acrn_vcpu *vcpu, bool guest_irq_enabled, bool injected);

#endif /* __RISCV_VIRQ_H__ */
