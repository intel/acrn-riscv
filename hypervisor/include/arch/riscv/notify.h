#ifndef __RISCV_NOTIFY_H__
#define __RISCV_NOTIFY_H__
#include <acrn/cpumask.h>

typedef void (*smp_call_func_t)(void *data);
struct smp_call_info_data {
	smp_call_func_t func;
	void *data;
};

void smp_call_function(cpumask_t mask, smp_call_func_t func, void *data);
void smp_call_init(void);
void kick_notification(void);

#endif
