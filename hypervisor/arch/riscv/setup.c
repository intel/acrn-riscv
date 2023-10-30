/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#include <types.h>
#include <asm/setup.h>
#include <asm/page.h>
#include <asm/early_printk.h>
#include <debug/logmsg.h>
#include <asm/smp.h>
#include <asm/notify.h>
#include <acrn/cpumask.h>

struct bootinfo bootinfo;

unsigned int nr_cpu_ids = NR_CPUS;
#define MASK_DECLARE_1(x) [x+1][0] = 1UL << (x)
#define MASK_DECLARE_2(x) MASK_DECLARE_1(x), MASK_DECLARE_1(x+1)
#define MASK_DECLARE_4(x) MASK_DECLARE_2(x), MASK_DECLARE_2(x+2)
#define MASK_DECLARE_8(x) MASK_DECLARE_4(x), MASK_DECLARE_4(x+4)


const unsigned long cpu_bit_bitmap[BITS_PER_LONG+1][BITS_TO_LONGS(NR_CPUS)] = {

    MASK_DECLARE_8(0),  MASK_DECLARE_8(8),
    MASK_DECLARE_8(16), MASK_DECLARE_8(24),
#if BITS_PER_LONG > 32
    MASK_DECLARE_8(32), MASK_DECLARE_8(40),
    MASK_DECLARE_8(48), MASK_DECLARE_8(56),
#endif
};


extern struct acrn_vm *sos_vm;
size_t dcache_line_bytes;
/* C entry point for boot CPU */
void start_acrn(uint32_t cpu, unsigned long boot_phys_offset,
                      unsigned long fdt_paddr)
{
	set_current(&idle_vcpu[cpu]);
	set_processor_id(cpu); /* needed early, for smp_processor_id() */
	init_logmsg(CONFIG_LOG_LEVEL);
	setup_pagetables(boot_phys_offset);
	dcache_line_bytes = read_dcache_line_bytes();

	percpu_init_areas();
	pr_info("start acrn, boot_phys_offset = 0x%lx\n", boot_phys_offset);
	init_trap();

	init_IRQ();
	preinit_timer();
	plic_init();
//	init_pcpu_capabilities();
//	ASSERT(detect_hardware_support() == 0);

	smp_clear_cpu_maps();
	smp_init_cpus();
	start_pcpus();
	pr_info("Brought up %ld CPUs\n", (long)num_online_cpus());
	smp_call_init();

	timer_init();
	pr_info("init timer\r\n");


	console_init();
	console_setup_timer();
	pr_info("console init \r\n");
	local_irq_enable();
	setup_virt_paging();

	init_sched(0U);

	pr_info("prepare sos");
	prepare_sos_vm();

	pr_info("create vm");
	create_vm(sos_vm);

	// currently, direct run sos.
//	release_irq(26);
	start_sos_vm();
	pr_info("end\n");

	run_idle_thread();
	while(1);

	asm volatile("wfi" : : : "memory");
}
