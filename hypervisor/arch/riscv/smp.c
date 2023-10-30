#include "uart.h"
#include <acrn/percpu.h>
#include <acrn/cpumask.h>
#include <schedule.h>
#include <asm/smp.h>

void cpu_do_idle(void)
{
	asm volatile ("wfi"::);
}

int do_swi(int cpu)
{
	cpu *= 4;
	cpu += 0x02000000;
	asm volatile (
		"li a0, 1\n\t" \
		"sw a0, 0(%0) \n\t"
		:: "r"(cpu)
	);

	return 0;
}

int g_cpus = 1;
void smp_start_cpus()
{
	for (int i = 1; i < g_cpus; i++) {
		do_swi(i);
	}
}

void cpu_dead(void)
{
	while(1);
}

int get_pcpu_nums(void)
{
	return g_cpus;
}


