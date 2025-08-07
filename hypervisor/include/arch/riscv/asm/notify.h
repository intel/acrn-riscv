/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_NOTIFY_H__
#define __RISCV_NOTIFY_H__

typedef void (*smp_call_func_t)(void *data);
struct smp_call_info_data {
	smp_call_func_t func;
	void *data;
};

extern void smp_call_function(uint64_t mask, smp_call_func_t func, void *data);
extern void smp_call_init(void);
extern void kick_notification(void);

#endif /* __RISCV_NOTIFY_H__ */
