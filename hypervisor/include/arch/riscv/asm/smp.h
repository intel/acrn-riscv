/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_SMP_H__
#define __RISCV_SMP_H__

#ifndef __ASSEMBLY__
#include <asm/current.h>

#define CLINT_SWI_REG	CONFIG_CLINT_BASE
struct swi_vector {
	uint64_t type;
	uint64_t param;
};
#endif /* !__ASSEMBLY__ */

#define smp_processor_id() get_pcpu_id()

extern void init_secondary(void);
extern void stop_cpu(void);
extern void smp_init_cpus(void);
extern void smp_clear_cpu_maps (void);
extern void start_pcpus(uint32_t cpu);
extern int kick_pcpu(int cpu);
extern int smp_platform_init(void);

struct smp_ops {
	int (*kick_cpu)(int cpu);
	void (*send_single_swi)(uint16_t pcpu_id, uint64_t vector);
	void (*send_dest_ipi_mask)(uint64_t dest_mask, uint64_t vector);
	int (*ipi_start_cpu)(int cpu, uint64_t addr, uint64_t arg);
	void (*rfence)(uint64_t dest_mask, uint64_t addr, uint64_t size);
	void (*hfence)(uint64_t dest_mask, uint64_t addr, uint64_t size);
};

extern void register_smp_ops(struct smp_ops *ops);
extern struct smp_ops *smp_ops;

#endif /* __RISCV_SMP_H__ */
