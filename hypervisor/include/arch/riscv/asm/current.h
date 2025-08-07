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
	current->pcpu_id = id;	\
} while ( 0 )

static inline struct cpu_info *get_cpu_info(void)
{
#ifdef __clang__
	unsigned long sp;

	asm ("mv %0, sp" : "=r" (sp));
#else
	register unsigned long sp asm ("sp");
#endif

	return (struct cpu_info *)((sp & ~(STACK_SIZE - 1)) +
		STACK_SIZE - sizeof(struct cpu_info));
}

#define guest_cpu_ctx_regs() (&get_cpu_info()->guest_cpu_ctx_regs)

#define switch_stack_and_jump(stack, fn) \
	asm volatile ("mv sp, %0; jalr %1" : : "r" (stack), "r" (fn) : "memory" )

#define reset_stack_and_jump(fn) switch_stack_and_jump(get_cpu_info(), fn)

#endif /* !__ASSEMBLY__ */

#endif /* __RISCV_CURRENT_H__ */
