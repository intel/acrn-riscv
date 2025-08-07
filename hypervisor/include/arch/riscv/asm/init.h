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

#include <types.h>

#define SP_BOTTOM_MAGIC		0x696e746cUL
struct init_info
{
	unsigned char *stack;
	/* Logical CPU ID, used by start_secondary */
	unsigned int cpuid;
};

extern void init_IRQ(void);
extern void plic_init(void);
extern void prepare_sos_vm(void);
extern void prepare_uos_vm(void);
extern void init_trap(void);

extern char _start[], _end[], start[], _boot[], _vboot[];
#define is_kernel(p) ({					\
	char *__p = (char *)(unsigned long)(p);		\
	(__p >= _start) && (__p < _end);		\
})

extern char _stext[], _etext[];
#define is_kernel_text(p) ({				\
	char *__p = (char *)(unsigned long)(p);		\
	(__p >= _stext) && (__p < _etext);		\
})

extern const char _srodata[], _erodata[];
#define is_kernel_rodata(p) ({					\
	const char *__p = (const char *)(unsigned long)(p);	\
	(__p >= _srodata) && (__p < _erodata);			\
})

extern char _sinittext[], _einittext[];
#define is_kernel_inittext(p) ({			\
	char *__p = (char *)(unsigned long)(p);		\
	(__p >= _sinittext) && (__p < _einittext);	\
})

#define min_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#define max_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })

/*
 * Mark functions and data as being only used at initialization
 * or exit time.
 */
#define __init			__attribute__((__section__(".init.text")))
#define __exit			__attribute__((__section__(".exit.text")))
#define __cold			__attribute__((__section__(".text.cold")))
#define __initdata		__attribute__((__section__(".init.data")))
#define __initconst		__attribute__((__section__(".init.rodata")))
#define __initconstrel		__atrribute__((__section__(".init.rodata.rel")))
#define __exitdata		__attribute__((__section__(".exit.data")))
#define __initsetup		__attribute__((__section__(".init.setup")))
#define __init_call(lvl)	__attribute__((__section(".initcall" lvl ".init")))
#define __exit_call		__attribute__((_section(".exitcall.exit")))

#ifndef __ASSEMBLY__

typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

#define presmp_initcall(fn) \
	const static initcall_t __initcall_##fn __init_call("presmp") = fn
#define __initcall(fn) \
	const static initcall_t __initcall_##fn __init_call("1") = fn
#define __exitcall(fn) \
	static exitcall_t __exitcall_##fn __exit_call = fn

void do_presmp_initcalls(void);
void do_initcalls(void);

#endif /* !__ASSEMBLY__ */

#endif /* __RISCV_INIT_H__ */
