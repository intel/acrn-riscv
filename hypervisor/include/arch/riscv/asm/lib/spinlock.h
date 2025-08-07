/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_LIB_SPINLOCK_H__
#define __RISCV_LIB_SPINLOCK_H__

#ifndef __ASSEMBLY__
#include <rtl.h>
#include <asm/types.h>
#include <asm/system.h>

typedef struct _spinlock {
	uint64_t head;
	uint64_t tail;
} spinlock_t;

static inline void spinlock_init(spinlock_t *lock)
{
	(void)memset(lock, 0U, sizeof(spinlock_t));
}

static inline void spinlock_obtain(spinlock_t *lock)
{
	asm volatile ("   li t0, 0x1\n\t"
		      "   amoadd.d t1, t0, (%[head])\n\t"
		      "2: ld t0, (%[tail])\n\t"
		      "   beq t1, t0, 1f\n\t"
		      "   j 2b\n\t"
		      "1:\n"
		      :
		      :
		      [head] "r"(&lock->head),
		      [tail] "r"(&lock->tail)
		      : "cc", "memory", "t0", "t1");
}

static inline void spinlock_release(spinlock_t *lock)
{
	asm volatile ("   li t0, 0x1\n\t"
		      "   amoadd.d t1, t0, (%[tail])\n"
		      :
		      : [tail] "r" (&lock->tail)
		      : "cc", "memory", "t0", "t1");
}

#else /* __ASSEMBLY__ */

#define SPINLOCK_HEAD_OFFSET       0
#define SPINLOCK_TAIL_OFFSET       4

.macro spinlock_obtain lock
	li t0, 0x1
	li t2, lock
	amoadd.d t1, t0, SPINLOCK_HEAD_OFFSET(t2)
2:	ld t0, SPINLOCK_TAIL_OFFSET(t2)
	beq t1, t0, 1f
	j 2b
1 :
.endm

#define spinlock_obtain(x) spinlock_obtain lock = (x)

.macro spinlock_release lock
	li t0, 0x1
	li t2, lock
	amoadd.d t1, t0, SPINLOCK_TAIL_OFFSET(t2)
.endm

#define spinlock_release(x) spinlock_release lock = (x)

#endif	/* __ASSEMBLY__ */

#define spinlock_irqsave_obtain(lock, flags)		\
	do {						\
		local_irq_save(flags);			\
		spinlock_obtain(lock);			\
	} while (0)

#define spinlock_irqrestore_release(lock, flags)	\
	do {						\
		spinlock_release(lock);			\
		local_irq_restore(flags);		\
	} while (0)

#define spin_lock(x) spinlock_obtain(x)
#define spin_unlock(x) spinlock_release(x)

#define spin_lock_irqsave(l, f) spinlock_irqsave_obtain(l, f)
#define spin_unlock_irqrestore(l, f) spinlock_irqrestore_release(l, f)

#endif /* __RISCV_LIB_SPINLOCK_H__ */
