#ifndef __RISCV_CURRENT_H__
#define __RISCV_CURRENT_H__

#ifndef __ASSEMBLY__

#include <acrn/percpu.h>
#include <asm/cpu.h>
#include <asm/guest/vcpu.h>

register struct acrn_vcpu *curr_vcpu __asm__("tp");
static inline struct acrn_vcpu *get_current(void)
{
	return curr_vcpu;
}

static inline void set_current(struct acrn_vcpu *vcpu)
{
	curr_vcpu = vcpu;
}
#define current                 get_current()

#define get_processor_id()     (current->pcpu_id)

#define set_processor_id(id)                            \
do {                                                    \
	current->pcpu_id = id;				\
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

#define switch_stack_and_jump(stack, fn)                                \
    asm volatile ("mv sp,%0; b " STR(fn) : : "r" (stack) : "memory" )

#define reset_stack_and_jump(fn) switch_stack_and_jump(get_cpu_info(), fn)

#endif

#endif /* __RISCV_CURRENT_H__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
