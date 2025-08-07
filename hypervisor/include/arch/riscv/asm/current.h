/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_CURRENT_H__
#define __RISCV_CURRENT_H__

#ifndef __ASSEMBLY__
#include <asm/cpu.h>
#include <asm/guest/vcpu.h>
#include <debug/logmsg.h>

volatile register struct thread_object *current asm ("tp");
static inline void set_current(struct thread_object *obj)
{
	current = obj;
}

static inline uint16_t get_pcpu_id(void)
{
	ASSERT(current != 0);
	return current->pcpu_id;
}

#define set_pcpu_id(id)			\
do {					\
	current->pcpu_id = id;		\
} while(0)

#endif /* !__ASSEMBLY__ */

#endif /* __RISCV_CURRENT_H__ */
