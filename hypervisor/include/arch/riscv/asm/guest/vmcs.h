/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_VMCS_H__
#define __RISCV_VMCS_H__

#define VM_SUCCESS		0
#define VM_FAIL			-1

#ifndef __ASSEMBLY__

#include <types.h>
#include <asm/guest/vcpu.h>

#define HX_VMENTRY_FAIL		0x80000000U

extern void init_vmcs(struct acrn_vcpu *vcpu);
extern void load_vmcs(struct acrn_vcpu *vcpu);
extern void save_vmcs(struct acrn_vcpu *vcpu);

#endif /* __ASSEMBLY__ */

#endif /* __RISCV_VMCS_H__ */
