/*
 * Copyright (C) 2025 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_FLOAT_H__
#define __RISCV_FLOAT_H__

#ifdef CONFIG_MACRN
static inline void init_float(void) {};
#else
extern void init_float(void);
#endif

#endif
