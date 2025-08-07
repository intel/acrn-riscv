/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_VMEXIT_H__
#define __RISCV_VMEXIT_H__

struct vm_exit_dispatch {
	int32_t (*handler)(struct acrn_vcpu *);
	uint32_t need_exit_qualification;
};

extern int32_t vmexit_handler(struct acrn_vcpu *vcpu);
extern int32_t vmcall_vmexit_handler(struct acrn_vcpu *vcpu);
extern int32_t sbi_ecall_vmexit_handler(struct acrn_vcpu *vcpu);
extern int32_t rdcsr_vmexit_handler(struct acrn_vcpu *vcpu);
extern int32_t wrcsr_vmexit_handler(struct acrn_vcpu *vcpu);
extern void vm_exit(void);

#endif /* __RISCV_VMEXIT_H__ */
