#ifndef __RISCV_SYSTEM_H__
#define __RISCV_SYSTEM_H__

#include <acrn/lib.h>
#include <public/arch-riscv.h>

#define sev()           asm volatile("nop" : : : "memory")
#define wfe()           asm volatile("nop" : : : "memory")
#define wfi()           asm volatile("wfi" : : : "memory")

#define isb()           asm volatile("fence.i" : : : "memory")
#define dsb(scope)      asm volatile("fence" : : : "memory")
#define dmb(scope)      asm volatile("fence" : : : "memory")

#define mb()            dsb(sy)
#define rmb()           dsb(ld)
#define wmb()           dsb(st)

#define smp_mb()        dmb(ish)
#define smp_rmb()       dmb(ishld)

#define smp_wmb()       dmb(ishst)

#define smp_mb__before_atomic()    smp_mb()
#define smp_mb__after_atomic()     smp_mb()

/*
 * This is used to ensure the compiler did actually allocate the register we
 * asked it for some inline assembly sequences.  Apparently we can't trust
 * the compiler from one version to another so a bit of paranoia won't hurt.
 * This string is meant to be concatenated with the inline asm string and
 * will cause compilation to stop on mismatch.
 * (for details, see gcc PR 15089)
 */
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

# include <asm/riscv64/system.h>

static inline int local_abort_is_enabled(void)
{
    return -1;
}

#define arch_fetch_and_add(x, v) __sync_fetch_and_add(x, v)

struct acrn_vcpu;
extern struct acrn_vcpu *__context_switch(struct acrn_vcpu *prev, struct acrn_vcpu *next);

/* Synchronizes all write accesses to memory */
static inline void cpu_write_memory_barrier(void)
{
	asm volatile ("fence\n" : : : "memory");
}

/* Synchronizes all read and write accesses to/from memory */
static inline void cpu_memory_barrier(void)
{
	asm volatile ("fence\n" : : : "memory");
}

#endif
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
