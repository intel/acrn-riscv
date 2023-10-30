#ifndef __RISCV_TIMER_H__
#define __RISCV_TIMER_H__

#include <asm/cpu.h>
#include <asm/system.h>
#include <acrn/time.h>
//#include <acrn/list.h>
#include <list.h>

typedef uint64_t cycles_t;

static inline cycles_t get_cycles (void)
{
        isb();
        return cpu_csr_read(scounter);
}

/**
 * @brief Timer
 *
 * @defgroup timer ACRN Timer
 * @{
 */

typedef void (*timer_handle_t)(void *data);

/**
 * @brief Definition of timer tick mode
 */
enum tick_mode {
	TICK_MODE_ONESHOT = 0,	/**< one-shot mode */
	TICK_MODE_PERIODIC,	/**< periodic mode */
};
/**
 * @brief Definition of timers for per-cpu
 */
struct per_cpu_timers {
	struct list_head timer_list;	/**< it's for runtime active timer list */
};

/**
 * @brief Definition of timer
 */
struct hv_timer {
	struct list_head node;		/**< link all timers */
	enum tick_mode mode;		/**< timer mode: one-shot or periodic */
	uint64_t fire_tick;		/**< tick deadline to interrupt */
	uint64_t period_in_cycle;	/**< period of the periodic timer in unit of tick cycles */
	timer_handle_t func;		/**< callback if time reached */
	void *priv_data;		/**< func private data */
};

/**
 * @brief Initialize a timer structure.
 *
 * @param[in] timer Pointer to timer.
 * @param[in] func irq callback if time reached.
 * @param[in] priv_data func private data.
 * @param[in] fire_tick tick deadline to interrupt.
 * @param[in] mode timer mode.
 * @param[in] period_in_cycle period of the periodic timer in unit of tick cycles.
 *
 * @remark Don't initialize a timer twice if it has been added to the timer list
 *         after calling add_timer. If you want to, delete the timer from the list first.
 *
 * @return None
 */
static inline void initialize_timer(struct hv_timer *timer,
				timer_handle_t func, void *priv_data,
				uint64_t fire_tick, int32_t mode, uint64_t period_in_cycle)
{
	if (timer != NULL) {
		timer->func = func;
		timer->priv_data = priv_data;
		timer->fire_tick = fire_tick;
		timer->mode = mode;
		timer->period_in_cycle = period_in_cycle;
		INIT_LIST_HEAD(&timer->node);
	}
}

/* Counter value at boot time */
extern uint64_t boot_count;

extern s_time_t ticks_to_us(uint64_t ticks);
extern uint64_t us_to_ticks(s_time_t ns);

void udelay(s_time_t us);
unsigned long get_tick(void);
/**
 * @brief Add a timer.
 *
 * @param[in] timer Pointer to timer.
 *
 * @retval 0 on success
 * @retval -EINVAL timer has an invalid value
 *
 * @remark Don't call it in the timer callback function or interrupt content.
 */
int32_t add_timer(struct hv_timer *timer);

/**
 * @brief Delete a timer.
 *
 * @param[in] timer Pointer to timer.
 *
 * @return None
 *
 * @remark Don't call it in the timer callback function or interrupt content.
 */
void del_timer(struct hv_timer *timer);

/**
 * @brief Initialize timer.
 *
 * @return None
 */
void preinit_timer(void);
void timer_init(void);
void hv_timer_handler(struct cpu_user_regs *regs);
#define CYCLES_PER_MS	us_to_ticks(1000U)

#define CLINT_MTIME		(CONFIG_CLINT_BASE + 0xBFF8)
#define CLINT_MTIMECMP(cpu)	(CONFIG_CLINT_BASE + 0x4000 + (cpu * 0x8))
#define CLINT_DISABLE_TIMER	0xffffffff

#endif /* __RISCV_TIMER_H__ */
