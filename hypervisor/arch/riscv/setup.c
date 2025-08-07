/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <types.h>
#include <irq.h>
#include <asm/init.h>
#include <asm/setup.h>
#include <asm/cache.h>
#include <asm/cpumask.h>
#include <asm/mem.h>
#include <asm/float.h>
#include <asm/early_printk.h>
#include <asm/smp.h>
#include <asm/per_cpu.h>
#include <asm/notify.h>
#include <asm/guest/vm.h>
#include <asm/guest/s2vm.h>
#include <debug/console.h>
#include <debug/logmsg.h>
#include <debug/shell.h>

size_t dcache_block_size;

/* C entry point for boot CPU */
void start_acrn(uint32_t cpu, unsigned long boot_phys_offset,
                      unsigned long fdt_paddr)
{
	struct thread_object *idle = &per_cpu(idle, cpu);

	init_percpu_areas();
	set_current(idle);
	set_pcpu_id(cpu); /* needed early, for smp_processor_id() */
	smp_clear_cpu_maps();
	set_bit(cpu, &cpu_online_map);
	init_logmsg();

	setup_mem(boot_phys_offset);
	dcache_block_size = get_dcache_block_size();

	pr_info("start acrn, boot_phys_offset = 0x%lx\n", boot_phys_offset);
#ifdef CONFIG_MACRN
	init_mtrap();
#else
	init_trap();
	init_float();
#endif
	init_interrupt(BSP_CPU_ID);
	preinit_timer();
	plic_init();
//	init_pcpu_capabilities();
//	ASSERT(detect_hardware_support() == 0);

	timer_init();
	pr_info("init timer\r\n");
	console_init();
	if (cpu == 4) {
		shell_init();
		console_setup_timer();
	}
//	console_setup_timer();
	pr_info("console init \r\n");

	smp_platform_init();
	start_pcpus(cpu);
	pr_info("Brought up %ld CPUs\n", (long)num_online_cpus());
	smp_call_init();

	setup_virt_paging();
	init_sched(cpu);

	pr_info("prepare sos");
	prepare_sos_vm();
	pr_info("create vm");
	create_vm(sos_vm);
	// currently, direct run sos.
	start_vm(sos_vm);

#if defined(CONFIG_KTEST) || defined(CONFIG_UOS)
	prepare_uos_vm();
	create_vm(uos_vm);
	start_vm(uos_vm);
#endif
	local_irq_enable();

	run_idle_thread();
	while(1)
		asm volatile("wfi" : : : "memory");
}
