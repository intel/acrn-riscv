/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#include <types.h>
#include <asm/guest/vcpu.h>

void profiling_vmenter_handler(__unused struct acrn_vcpu *vcpu) {}
void profiling_pre_vmexit_handler(__unused struct acrn_vcpu *vcpu) {}
void profiling_post_vmexit_handler(__unused struct acrn_vcpu *vcpu) {}
void profiling_setup(void) {}
