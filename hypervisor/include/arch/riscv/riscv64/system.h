#ifndef __RISCV64_SYSTEM_H__
#define __RISCV64_SYSTEM_H__

#define local_irq_disable()   asm volatile ( "csrc sstatus, 0x2\n" :::)
#define local_irq_enable()    asm volatile ( "csrs sstatus, 0x2\n" :::)

#define local_save_flags(x)					\
({								\
	BUILD_BUG_ON(sizeof(x) != sizeof(long));		\
	asm volatile ( "csrr %0, sstatus\n" :"=r"(x):: "memory");	\
})

#define local_irq_save(x)					\
({								\
	local_save_flags(x);					\
	local_irq_disable();					\
})

#define local_irq_restore(x)					\
({								\
	BUILD_BUG_ON(sizeof(x) != sizeof(long));		\
	asm volatile ( "csrw sstatus, %0\n" ::"r"(x): "memory");	\
})

static inline int local_irq_is_enabled(void)
{
	unsigned long flags;
	local_save_flags(flags);
	return !(flags & 0x2);
}

#endif
