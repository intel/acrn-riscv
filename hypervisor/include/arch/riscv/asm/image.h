/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_IMAGE_H__
#define __RISCV_IMAGE_H__

#include <asm/guest/vm.h>

extern void get_kernel_info(struct kernel_info *info);
extern void get_dtb_info(struct dtb_info *info);

#endif
