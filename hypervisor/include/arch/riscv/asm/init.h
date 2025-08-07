/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_INIT_H__
#define __RISCV_INIT_H__

#include <asm/types.h>

#define SP_BOTTOM_MAGIC		0x696e746cUL
struct init_info
{
	uint8_t *stack;
	/* Logical CPU ID, used by start_secondary */
	uint32_t cpuid;
};

extern void init_IRQ(void);
extern void plic_init(void);
extern void init_trap(void);
extern void init_mtrap(void);

extern uint8_t _start[], _end[], _boot[];
#define is_hv(addr) ({					\
	uint8_t *p = (uint8_t *)(uint64_t)(addr);	\
	(p >= _start) && (p < _end);			\
})

extern uint8_t _stext[], _etext[];
#define is_hv_text(addr) ({				\
	uint8_t *p = (uint8_t *)(uint64_t)(addr);	\
	(p >= _stext) && (p < _etext);			\
})

/* Not using __init yet, just a placeholder */
#define __init	__attribute__((__section__(".init.text")))

#endif /* __RISCV_INIT_H__ */
