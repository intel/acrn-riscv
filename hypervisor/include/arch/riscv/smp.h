#ifndef __RISCV_SMP_H__
#define __RISCV_SMP_H__

#ifndef __ASSEMBLY__
#include <asm/current.h>
#include <acrn/cpumask.h>
#endif


#define smp_processor_id() get_processor_id()

typedef void (*smp_call_func_t)(void *data);
extern void smp_call_function(cpumask_t mask, smp_call_func_t func, void *data);

/*
 * Do we, for platform reasons, need to actually keep CPUs online when we
 * would otherwise prefer them to be off?
 */
#define park_offline_cpus false

extern void init_secondary(void);
extern void stop_cpu(void);

/* Secondary CPU entry point */

extern void smp_init_cpus(void);
extern void smp_clear_cpu_maps (void);

void start_pcpus(void);

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
