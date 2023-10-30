#include <public/irq.h>
#include <asm/timer.h>
#include <asm/current.h>
#include <debug/logmsg.h>
//#include <asm/softirq.h>
#include <riscv/riscv64/system.h>
#include <riscv/riscv64/io.h>
#include <acrn/percpu.h>

unsigned long cpu_khz;  /* CPU clock frequency in kHz. */
unsigned long boot_count;
static DEFINE_PER_CPU(struct per_cpu_timers, cpu_timers);

#define MAX_TIMER_ACTIONS	32U
#define CAL_MS			10U
#define MIN_TIMER_PERIOD_US	500U


/* Qemu default cpu freq is 0x10000000 */
#define QEMU_CPUFREQ		0x10000000

static unsigned long get_cpu_khz(void)
{
	return QEMU_CPUFREQ / 1000;
}

unsigned long get_tick(void)
{
	return readq_relaxed(CLINT_MTIME);
}

s_time_t ticks_to_us(uint64_t ticks)
{
	return ticks * 1000 / cpu_khz;
}

uint64_t us_to_ticks(s_time_t us)
{
	return us * cpu_khz / 1000;
}

/* Return number of nanoseconds since boot */
s_time_t get_s_time(void)
{
	uint64_t ticks = get_tick();
	return ticks_to_us(ticks);
}

void set_deadline(uint64_t deadline)
{
	uint16_t cpu = get_processor_id();
	if (deadline < get_tick()) {
		pr_dbg("deadline not correct");
		return;
	}

	writeq_relaxed(deadline, CLINT_MTIMECMP(cpu));
	//isb();

	return;
}

void udelay(s_time_t us)
{
	uint64_t deadline;

	deadline = us_to_ticks(us) + get_tick();
	while (get_tick() < deadline);
	dsb(sy);
	isb();
}

static inline void update_physical_timer(struct per_cpu_timers *cpu_timer)
{
	struct hv_timer *timer = NULL;

	/* find the next event timer */
	if (!list_empty(&cpu_timer->timer_list)) {
		timer = container_of((&cpu_timer->timer_list)->next,
			struct hv_timer, node);

		/* it is okay to program a expired time */
		set_deadline(timer->fire_tick);
	}
}
/*
 * return true if we add the timer on the timer_list head
 */
static bool local_add_timer(struct per_cpu_timers *cpu_timer,
			struct hv_timer *timer)
{
	struct list_head *pos, *prev;
	struct hv_timer *tmp;
	uint64_t tick = timer->fire_tick;

	prev = &cpu_timer->timer_list;
	list_for_each(pos, &cpu_timer->timer_list) {
		tmp = container_of(pos, struct hv_timer, node);
		if (tmp->fire_tick < tick) {
			prev = &tmp->node;
		}
		else {
			break;
		}
	}

	list_add(&timer->node, prev);

	return (prev == &cpu_timer->timer_list);
}

void del_timer(struct hv_timer *timer)
{
	uint64_t rflags;

	local_irq_save(rflags);
	if ((timer != NULL) && !list_empty(&timer->node)) {
		list_del_init(&timer->node);
	}
	local_irq_restore(rflags);
}

int32_t add_timer(struct hv_timer *timer)
{
	struct per_cpu_timers *cpu_timer;
	uint16_t pcpu_id;
	int32_t ret = 0;
	uint64_t rflags;

	if ((timer == NULL) || (timer->func == NULL) || (timer->fire_tick == 0UL)) {
		ret = -1;
	} else {
		ASSERT(list_empty(&timer->node));

		/* limit minimal periodic timer cycle period */
		if (timer->mode == TICK_MODE_PERIODIC) {
			timer->period_in_cycle = max(timer->period_in_cycle, us_to_ticks(MIN_TIMER_PERIOD_US));
		}

		pcpu_id  = get_processor_id();
		cpu_timer = &per_cpu(cpu_timers, pcpu_id);

		local_irq_save(rflags);
		/* update the physical timer if we're on the timer_list head */
		pr_dbg("add timer ");
		if (local_add_timer(cpu_timer, timer)) {
			update_physical_timer(cpu_timer);
		}
		pr_dbg("add timer finish");
		local_irq_restore(rflags);
	}

	return ret;

}

static void run_timer(const struct hv_timer *timer)
{
	/* deadline = 0 means stop timer, we should skip */
	if ((timer->func != NULL) && (timer->fire_tick != 0UL)) {
		timer->func(timer->priv_data);
	}

}

static void timer_deadline_handler(uint16_t pcpu_id)
{
	struct per_cpu_timers *cpu_timer;
	struct hv_timer *timer;
	struct list_head *pos, *n;
	uint32_t tries = MAX_TIMER_ACTIONS;
	uint64_t current_tick = get_tick();

	/* handle passed timer */
	cpu_timer = &per_cpu(cpu_timers, pcpu_id);

	/* This is to make sure we are not blocked due to delay inside func()
	 * force to exit irq handler after we serviced >31 timers
	 * caller used to local_add_timer() for periodic timer, if there is a delay
	 * inside func(), it will infinitely loop here, because new added timer
	 * already passed due to previously func()'s delay.
	 */
	list_for_each_safe(pos, n, &cpu_timer->timer_list) {
		timer = container_of(pos, struct hv_timer, node);
		/* timer expried */
		tries--;
		if ((timer->fire_tick <= current_tick) && (tries != 0U)) {
			del_timer(timer);

			run_timer(timer);

			if (timer->mode == TICK_MODE_PERIODIC) {
				/* update periodic timer fire tick */
				timer->fire_tick = get_tick() + timer->period_in_cycle;
				(void)local_add_timer(cpu_timer, timer);
			}
		} else {
			break;
		}
	}

	/* update nearest timer */
	update_physical_timer(cpu_timer);
}

void hv_timer_handler(struct cpu_user_regs *regs)
{
	timer_deadline_handler(get_processor_id());
}

void preinit_timer()
{
	cpu_khz = get_cpu_khz();
	boot_count = get_tick();
	pr_dbg("cpu_khz = %ld boot_count=%ld \r\n", cpu_khz, boot_count);

	for (int i = 0; i < NR_CPUS; i++) {
		writeq_relaxed(CLINT_DISABLE_TIMER, CLINT_MTIMECMP(i));
	}
}

void timer_init(void)
{
	uint16_t pcpu_id = get_processor_id();
	struct per_cpu_timers *cpu_timer;

	isb();

	cpu_timer = &per_cpu(cpu_timers, pcpu_id);
	INIT_LIST_HEAD(&(cpu_timer->timer_list));
}

uint64_t rdtsc(void)
{
	return get_tick();
}
