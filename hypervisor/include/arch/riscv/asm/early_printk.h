/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_EARLY_PRINTK_H__
#define __RISCV_EARLY_PRINTK_H__

extern void early_putch(char *c);
extern char early_getch(void);

static inline void early_flush(void) {}

#endif /* __RISCV_EARLY_PRINTK_H__ */
