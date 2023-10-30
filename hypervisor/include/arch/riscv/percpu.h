#ifndef __RISCV_PERCPU_H__
#define __RISCV_PERCPU_H__

#ifndef __ASSEMBLY__

#include <acrn/types.h>
//#include <asm/current.h>

extern char __per_cpu_start[], __per_cpu_data_end[];
extern unsigned long __per_cpu_offset[NR_CPUS];
void percpu_init_areas(void);

#define per_cpu(var, cpu)  \
    (*RELOC_HIDE(&per_cpu__##var, __per_cpu_offset[cpu]))
#define this_cpu(var) \
    (*RELOC_HIDE(&per_cpu__##var, get_processor_id()))

#define per_cpu_ptr(var, cpu)  \
    (*RELOC_HIDE(var, __per_cpu_offset[cpu]))
#define this_cpu_ptr(var) \
    (*RELOC_HIDE(var, get_processor_id()))

#endif

#endif /* __RISCV_PERCPU_H__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
