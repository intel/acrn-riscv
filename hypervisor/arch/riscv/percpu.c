#include <acrn/percpu.h>
#include <acrn/init.h>
#include <asm/page.h>
#include <acrn/mm.h>

#include <debug/logmsg.h>

unsigned long __per_cpu_offset[NR_CPUS];
#define INVALID_PERCPU_AREA (-(long)__per_cpu_start)
//#define PERCPU_ORDER (get_order_from_bytes(__per_cpu_data_end-__per_cpu_start))
#define PERCPU_ORDER	4U
#define PERCPU_SIZE  (PAGE_SIZE << PERCPU_ORDER)
static unsigned char __initdata per_cpu_data[NR_CPUS][PERCPU_SIZE] __attribute__((__aligned__(PERCPU_SIZE)));

void __init percpu_init_areas(void)
{
	unsigned int cpu;

	pr_dbg("%s, __per_cpu_start: 0x%lx, __per_cpu_data_end: 0x%lx\n",
					__func__, __per_cpu_start, __per_cpu_data_end);

	for ( cpu = 1; cpu < NR_CPUS; cpu++ ) {
		if (get_order_from_bytes(__per_cpu_data_end-__per_cpu_start) > PERCPU_ORDER) {
			__per_cpu_offset[cpu] = INVALID_PERCPU_AREA;
			pr_dbg("%s please enlarge PERCPU_ORDER from %d to %d\n", __func__,
					PERCPU_ORDER, get_order_from_bytes(__per_cpu_data_end-__per_cpu_start));
		} else {
			char *p = per_cpu_data[cpu];;
			memset(p, 0, __per_cpu_data_end - __per_cpu_start);
			__per_cpu_offset[cpu] = p - __per_cpu_start;
		}
	}
}
