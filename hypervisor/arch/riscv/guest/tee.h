/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_TEE_H__
#define __RISCV_TEE_H__

int32_t tee_switch(struct acrn_vcpu *vcpu);
int32_t tee_answer_ree(struct acrn_vcpu *vcpu);

#endif
