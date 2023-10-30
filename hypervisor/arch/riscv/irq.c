#include <public/irq.h>
#include <acrn/init.h>
#include <errno.h>
#include <debug/logmsg.h>
#include <asm/irq.h>
#include <acrn/bitmap.h>
#include <acrn/cpumask.h>

const unsigned int nr_irqs = NR_IRQS;

static struct irq_desc irq_desc[NR_IRQS];

struct acrn_irqchip_ops dummy_irqchip;

struct irq_desc *__irq_to_desc(int irq)
{
	return &irq_desc[irq];
}

static int arch_init_one_irq_desc(struct irq_desc *desc)
{
	desc->arch.type = IRQ_TYPE_INVALID;
	return 0;
}

/*
 * @ Could be common API
 * */
int init_one_irq_desc(struct irq_desc *desc)
{
	int err;

	if (irq_desc_initialized(desc)) {
		return 0;
	}

	set_bit(_IRQ_DISABLED, &desc->arch.status);
	desc->arch.irqchip = &dummy_irqchip;

	err = arch_init_one_irq_desc(desc);
	if (err) {
		desc->arch.irqchip = NULL;
	}

	return err;
}

/* only run on bsp boot*/
static void __init init_irq_data(void)
{
	int irq;

	for (irq = 0; irq < NR_IRQS; irq++) {
		struct irq_desc *desc = irq_to_desc(irq);
		init_one_irq_desc(desc);
		desc->irq = irq;
		desc->action  = NULL;
	}
}

void __init init_IRQ(void)
{
	init_irq_data();
}

int irq_set_type(unsigned int irq, unsigned int type)
{
	struct irq_desc *desc;

	desc = irq_to_desc(irq);
	desc->arch.type = type;
	return 0;
}

/*
 * currently, do not support irq sharing.
 */
int32_t request_irq(uint32_t irq, irq_action_t action_fn,
		void *priv_data, uint32_t flags)
{
	struct irq_desc *desc;

	if (irq >= nr_irqs || !action_fn) {
		return -EINVAL;
	}

	desc = irq_to_desc(irq);
	if (test_bit(_IRQ_GUEST, &desc->arch.status))
	{
		pr_dbg("IRQ %d is in used", irq);
		return -EBUSY;
	}
	if (desc->action != NULL)
		return -EINVAL;

	desc->action = action_fn;
	desc->priv_data = priv_data;

	desc->arch.irqchip->enable(desc);

	return 0;
}

/*
 * currently, do not support irq sharing.
 */
void release_irq(unsigned int irq)
{
	struct irq_desc *desc;
	unsigned long flags;

	desc = irq_to_desc(irq);
	if (!desc->action) {
		pr_dbg("irq %d is already freed", irq);
		return;
	}
	spin_lock_irqsave(&desc->lock, flags);
	clear_bit(_IRQ_GUEST, &desc->arch.status);
	spin_unlock_irqrestore(&desc->lock, flags);
	do {
		smp_mb();
	} while( test_bit(_IRQ_INPROGRESS, &desc->arch.status));
	desc->action = NULL;
}

void do_IRQ(struct cpu_regs *regs, unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	spin_lock(&desc->lock);

	//temporary code before virtual timer is implemented
	if(irq == 27) {
		desc->arch.irqchip->eoi(desc);
		set_bit(_IRQ_INPROGRESS, &desc->arch.status);
//		vclint_inject_irq(current->vm, current, irq, true);
		goto out_no_end;
	}

	if (test_bit(_IRQ_DISABLED, &desc->arch.status)) {
		pr_dbg("irq is disabled");
		goto out;
	}

	set_bit(_IRQ_INPROGRESS, &desc->arch.status);

	// run handler with irq enabled.
	spin_unlock_irq(&desc->lock);
	if (desc->action)
		desc->action(irq, regs);

	// disable irq
	spin_lock_irq(&desc->lock);
	clear_bit(_IRQ_INPROGRESS, &desc->arch.status);

out:
	desc->arch.irqchip->eoi(desc);
out_no_end:
	spin_unlock(&desc->lock);
}

struct acrn_irqchip_ops *acrn_irqchip;

void dispatch_interrupt(struct cpu_ctx_regs *regs)
{
	uint32_t irq;
	do {
		irq = acrn_irqchip->get_irq();
		if (irq == 0)
			break;
		pr_dbg("dispatch interrupt: %d",irq);
		do_IRQ(regs, irq);
	} while (1);
}
