#ifndef _IRQ_H__
#define _IRQ_H__

struct irq_desc;

#include <asm/irq.h>

typedef void (*irq_action_t)(uint32_t irq, void *priv_data);
struct irq_desc {
	uint32_t irq;
	irq_action_t action;
	void *priv_data;
	spinlock_t lock;
	struct arch_irq_desc arch;
};

#ifndef irq_to_desc
#define irq_to_desc(irq)    (&irq_desc[irq])
#endif

extern int32_t request_irq(uint32_t irq, irq_action_t action_fn, void *priv_data, uint32_t flags);

extern void release_irq(unsigned int irq);
#endif
