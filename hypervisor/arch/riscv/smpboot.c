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
	if (cpu == 4) {
		shell_init();
		console_setup_timer();
	}
	init_sched(cpu);

	local_irq_enable();
	run_idle_thread();
}

extern void secondary(void);
int kick_pcpu(int cpu)
{
	if (!smp_ops)
		return -ENODEV;

	return smp_ops->kick_cpu(cpu);
}

static int start_pcpu(int cpu)
{
	if (!smp_ops)
		return -ENODEV;

	return smp_ops->ipi_start_cpu(cpu, (uint64_t)secondary, 0);
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

	rc = start_pcpu(cpu);
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

void start_pcpus(uint32_t cpu)
{
	for (uint32_t i = 0; i < NR_CPUS; i++) {
		if (i != cpu)
			__cpu_up(i);
	}
}

void __init smp_init_cpus(void)
{
	for (int i = BSP_CPU_ID; i < NR_CPUS; i++) {
		set_bit(i, &cpu_possible_map);
		cpu_logical_map(i) = i;
	}
}

void stop_cpu(void)
{
	pr_dbg("%s", __func__);
}
