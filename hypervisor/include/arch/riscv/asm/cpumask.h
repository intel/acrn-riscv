/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_CPUMASK_H__
#define __RISCV_CPUMASK_H__

#include <asm/lib/bits.h>
extern uint64_t cpu_online_map;
extern uint64_t cpu_possible_map;

#if NR_CPUS > 1
#define num_online_cpus()	bit_weight(cpu_online_map)
#define num_possible_cpus()	bit_weight(cpu_possible_map)
#define cpu_online(cpu)		test_bit(cpu, cpu_online_map)
#define cpu_possible(cpu)	test_bit(cpu, cpu_possible_map)
#else
#define num_online_cpus()	1
#define num_possible_cpus()	1
#define cpu_online(cpu)		((cpu) == 0)
#define cpu_possible(cpu)	((cpu) == 0)
#endif

#endif /* __RISCV_CPUMASK_H__ */
