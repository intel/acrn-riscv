/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm/config.h>
#include <asm/current.h>
#include <asm/cpumask.h>
#include <asm/per_cpu.h>
#include <asm/init.h>
#include <asm/lib/bits.h>
#include <asm/types.h>
#include <asm/setup.h>
#include <asm/smp.h>
#include <asm/mem.h>
#include <asm/cache.h>
#include <asm/pgtable.h>
#include <asm/guest/vcpu.h>

#include <errno.h>
#include <debug/console.h>
#include <debug/logmsg.h>
#include <debug/shell.h>

struct acrn_vcpu idle_vcpu[NR_CPUS];

uint64_t cpu_online_map = 0UL;
uint64_t cpu_possible_map = 0UL;

struct smp_enable_ops {
	int (*prepare_cpu)(int);
};

static struct smp_enable_ops smp_enable_ops[NR_CPUS];

#define MP_INVALID_IDX 0xffffffff
uint64_t __cpu_logical_map[NR_CPUS] = { [0 ... NR_CPUS-1] = MP_INVALID_IDX};
#define cpu_logical_map(cpu) __cpu_logical_map[cpu]


static unsigned char __initdata cpu0_boot_stack[STACK_SIZE] __attribute__((__aligned__(STACK_SIZE)));

struct init_info init_data =
{
	.stack = cpu0_boot_stack,
};

/* Shared state for coordinating CPU bringup */
uint64_t smp_up_cpu = MP_INVALID_IDX;

void __init
smp_clear_cpu_maps (void)
{
/*
 * On Sophgo, it needs to explictly set the value since the very early
 * boot loader does not initialize .data section as zero.
 */
	cpu_online_map = 0UL;
	cpu_possible_map = 0UL;

	set_bit(0, &cpu_online_map);
	set_bit(0, &cpu_possible_map);
}

static int __init smp_platform_init(int cpu)
{
	smp_enable_ops[cpu].prepare_cpu = do_swi;
	return 0;
}

int __init arch_cpu_init(int cpu)
{
	return smp_platform_init(cpu);
}

int kick_pcpu(int cpu)
{
	if (!smp_enable_ops[cpu].prepare_cpu)
		return -ENODEV;

	return smp_enable_ops[cpu].prepare_cpu(cpu);
}

/* Bring up a remote CPU */
int __cpu_up(unsigned int cpu)
{
	int rc;

	pr_dbg("Bringing up CPU%d", cpu);

#ifndef CONFIG_MACRN
	rc = init_secondary_pagetables(cpu);
	if ( rc < 0 )
		return rc;
#endif

	/* Tell the remote CPU which stack to boot on. */
	init_data.stack = (unsigned char *)&idle_vcpu[cpu].stack;

	/* Tell the remote CPU what its logical CPU ID is. */
	init_data.cpuid = cpu;

	/* Open the gate for this CPU */
	smp_up_cpu = cpu_logical_map(cpu);
	clean_dcache(smp_up_cpu);

	rc = kick_pcpu(cpu);

	if ( rc < 0 )
	{
		pr_dbg("Failed to bring up CPU%d, rc = %d", cpu, rc);
		return rc;
	}

	while (!cpu_online(cpu))
	{
		cpu_relax();
	}

	smp_rmb();

	init_data.stack = NULL;
	init_data.cpuid = ~0;
	smp_up_cpu = MP_INVALID_IDX;
	clean_dcache(smp_up_cpu);

	if ( !cpu_online(cpu) )
	{
		pr_dbg("CPU%d never came online", cpu);
		return -EIO;
	}

	return 0;
}

void start_pcpus(void)
{
	for (uint32_t i = BSP_CPU_ID + 1U; i < NR_CPUS; i++) {
		__cpu_up(i);
	}
}

void __init smp_init_cpus(void)
{
	for (int i = BSP_CPU_ID; i < NR_CPUS; i++) {
		arch_cpu_init(i);
		set_bit(i, &cpu_possible_map);
		cpu_logical_map(i) = i;
	}
}

void start_secondary(uint32_t cpu)
{
	struct thread_object *idle = &per_cpu(idle, cpu);

	set_current(idle);
	set_pcpu_id(cpu);

	/* Now report this CPU is up */
	set_bit(cpu, &cpu_online_map);
#ifndef CONFIG_MACRN
	switch_satp(init_satp);
	init_trap();
#else
	init_mtrap();
#endif
	timer_init();
#if 0
	if (cpu == 4) {
		shell_init();
		console_setup_timer();
	}
#endif
	init_sched(cpu);

	local_irq_enable();
	run_idle_thread();
}

void stop_cpu(void)
{
	pr_dbg("%s", __func__);
}
