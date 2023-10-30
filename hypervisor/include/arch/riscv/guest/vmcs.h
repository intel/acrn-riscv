/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#ifndef __RISCV_VMCS_H__
#define __RISCV_VMCS_H__

#define VM_SUCCESS	0
#define VM_FAIL		-1

#ifndef ASSEMBLER
#include <types.h>
#include <asm/guest/vcpu.h>

#define HX_VMENTRY_FAIL                0x80000000U

void init_vmcs(struct acrn_vcpu *vcpu);
void load_vmcs(const struct acrn_vcpu *vcpu);

#define TYPE_INST_READ      (0UL << 12U)
#define TYPE_INST_WRITE     (1UL << 12U)

#endif /* ASSEMBLER */

#endif /* VMCS_H_ */
