#include <asm/config.h>
#include <asm/current.h>
#include <acrn/percpu.h>
#include <acrn/init.h>
#include <asm/types.h>
#include <asm/smp.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/guest/vcpu.h>
#include <acrn/cpumask.h>

#include <errno.h>
#include <debug/logmsg.h>

struct acrn_vcpu idle_vcpu[NR_CPUS];
//DEFINE_PER_CPU(struct acrn_vcpu *, curr_vcpu);

cpumask_t cpu_online_map;
cpumask_t cpu_possible_map;

struct smp_enable_ops {
		int (*prepare_cpu)(int);
};

static struct smp_enable_ops smp_enable_ops[NR_CPUS];

#define MP_INVALID_IDX 0xffffffff
/* CPU logical map: map cpuid to an MPIDR */
register_t __cpu_logical_map[NR_CPUS] = { [0 ... NR_CPUS-1] = MP_INVALID_IDX};
#define cpu_logical_map(cpu) __cpu_logical_map[cpu]


static unsigned char __initdata cpu0_boot_stack[STACK_SIZE] __attribute__((__aligned__(STACK_SIZE)));

struct init_info init_data =
{
	.stack = cpu0_boot_stack,
};

/* Shared state for coordinating CPU bringup */
uint64_t smp_up_cpu = MP_INVALID_IDX;

DEFINE_PER_CPU(unsigned int, cpu_id);

void __init
smp_clear_cpu_maps (void)
{
	cpumask_clear(&cpu_possible_map);
	cpumask_clear(&cpu_online_map);
	cpumask_set_cpu(0, &cpu_online_map);
	cpumask_set_cpu(0, &cpu_possible_map);
//	cpu_logical_map(0) = READ_SYSREG(MPIDR_EL1) & MPIDR_HWID_MASK;
}

extern int do_swi(int cpu);
static int __init smp_platform_init(int cpu)
{
	smp_enable_ops[cpu].prepare_cpu = do_swi;
	return 0;
}

int __init arch_cpu_init(int cpu)
{
	return smp_platform_init(cpu);
}

int arch_cpu_up(int cpu)
{
	if ( !smp_enable_ops[cpu].prepare_cpu )
		return -ENODEV;

	return smp_enable_ops[cpu].prepare_cpu(cpu);
}

/* Bring up a remote CPU */
int __cpu_up(unsigned int cpu)
{
	int rc;

	pr_dbg("Bringing up CPU%d", cpu);

	rc = init_secondary_pagetables(cpu);
	if ( rc < 0 )
		return rc;

	/* Tell the remote CPU which stack to boot on. */
	init_data.stack = &idle_vcpu[cpu].stack;

	/* Tell the remote CPU what its logical CPU ID is. */
	init_data.cpuid = cpu;

	/* Open the gate for this CPU */
	smp_up_cpu = cpu_logical_map(cpu);
	clean_dcache(smp_up_cpu);

	rc = arch_cpu_up(cpu);

	if ( rc < 0 )
	{
		pr_dbg("Failed to bring up CPU%d, rc = %d", cpu, rc);
		return rc;
	}

	while (!cpu_online(cpu))
	{
		cpu_relax();
	}

	/*
	 * Ensure that other cpus' initializations are visible before
	 * proceeding. Corresponds to smp_wmb() in start_secondary.
	 */
	smp_rmb();

	/*
	 * Nuke start of day info before checking one last time if the CPU
	 * actually came online. If it is not online it may still be
	 * trying to come up and may show up later unexpectedly.
	 *
	 * This doesn't completely avoid the possibility of the supposedly
	 * failed CPU trying to progress with another CPUs stack settings
	 * etc, but better than nothing, hopefully.
	 */

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
	for (uint32_t i = 1U; i < NR_CPUS; i++) {
		__cpu_up(i);
	}
}

void __init smp_init_cpus(void)
{
	for (int i = 0; i < NR_CPUS; i++) {
		arch_cpu_init(i);
		cpumask_set_cpu(i, &cpu_possible_map);
	#ifdef RUN_ON_QEMU
		cpu_logical_map(i) = i;
	#else
		cpu_logical_map(i) = i * 0x100U;
	#endif
	}
}

void init_local_irq_data(void);
/* Boot the current CPU */
void boot_idle(void);

void start_secondary(uint32_t cpuid)
{
	set_current(&idle_vcpu[cpuid]);
	set_processor_id(cpuid);

	/* Now report this CPU is up */
	cpumask_set_cpu(cpuid, &cpu_online_map);
	switch_satp(init_satp);
	pr_info("%s cpu = %d\n", __func__, cpuid);
	init_trap();
	pr_dbg("init traps");
	pr_dbg("init local irq");
	timer_init();

	init_sched(cpuid);

	local_irq_enable();
	run_idle_thread();
}

void stop_cpu(void)
{
	pr_dbg("%s", __func__);
}
