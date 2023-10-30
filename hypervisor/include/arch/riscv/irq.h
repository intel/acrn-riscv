#ifndef __RISCV_IRQ_H__
#define __RISCV_IRQ_H__

#include <asm/cpu.h>
#include <acrn/cpumask.h>
#include <acrn/spinlock.h>

#define IRQ_TYPE_NONE           0x00000000
#define IRQ_TYPE_EDGE_RISING    0x00000001
#define IRQ_TYPE_EDGE_FALLING   0x00000002
#define IRQ_TYPE_EDGE_BOTH      (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING)
#define IRQ_TYPE_LEVEL_HIGH     0x00000004
#define IRQ_TYPE_LEVEL_LOW      0x00000008
#define IRQ_TYPE_LEVEL_MASK     (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH)
#define IRQ_TYPE_SENSE_MASK     0x0000000f
#define IRQ_TYPE_INVALID	0x00000010

#define NR_VECTORS 256 /* XXX */

#define NOTIFY_VCPU_SWI		0
#define CLINT_SWI_EVENT_CHECK	1

/*
 * IRQ line status.
 */
#define _IRQ_INPROGRESS         0 /* IRQ handler active - do not enter! */
#define _IRQ_DISABLED           1 /* IRQ disabled - do not enter! */
#define _IRQ_PENDING            2 /* IRQ pending - replay on enable */
#define _IRQ_REPLAY             3 /* IRQ has been replayed but not acked yet */
#define _IRQ_GUEST              4 /* IRQ is handled by guest OS(es) */
#define _IRQ_MOVE_PENDING       5 /* IRQ is migrating to another CPUs */
#define _IRQ_PER_CPU            6 /* IRQ is per CPU */
#define _IRQ_GUEST_EOI_PENDING  7 /* IRQ was disabled, pending a guest EOI */
#define _IRQF_SHARED            8 /* IRQ is shared */
#define IRQ_INPROGRESS          (1u<<_IRQ_INPROGRESS)
#define IRQ_DISABLED            (1u<<_IRQ_DISABLED)
#define IRQ_PENDING             (1u<<_IRQ_PENDING)
#define IRQ_REPLAY              (1u<<_IRQ_REPLAY)
#define IRQ_GUEST               (1u<<_IRQ_GUEST)
#define IRQ_MOVE_PENDING        (1u<<_IRQ_MOVE_PENDING)
#define IRQ_PER_CPU             (1u<<_IRQ_PER_CPU)
#define IRQ_GUEST_EOI_PENDING   (1u<<_IRQ_GUEST_EOI_PENDING)
#define IRQF_SHARED             (1u<<_IRQF_SHARED)

/* Special IRQ numbers. */
#define AUTO_ASSIGN_IRQ         (-1)
#define NEVER_ASSIGN_IRQ        (-2)
#define FREE_TO_ASSIGN_IRQ      (-3)


typedef struct {
    DECLARE_BITMAP(_bits,NR_VECTORS);
} vmask_t;


struct irq_guest
{
    struct acrn_vm *vm;
    unsigned int virq;
};

struct acrn_irqchip_ops {
	const char *name;
	void (*init)(void);
	void (*set_irq_mask)(struct irq_desc *desc, uint32_t priority);
	void (*set_irq_priority)(struct irq_desc *desc, uint32_t priority);
	unsigned int (*get_irq)(void);
	void (*enable)(struct irq_desc *);
	void (*disable)(struct irq_desc *);
	void (*eoi)(struct irq_desc *desc);
};

extern struct acrn_irqchip_ops dummy_irqchip;

struct arch_irq_desc {
	unsigned int status;
	unsigned int type;
	struct irq_guest info;
	struct acrn_irqchip_ops *irqchip;
	cpumask_var_t affinity;
};
#define irq_desc_initialized(desc) ((desc)->arch.irqchip != NULL)

#define NR_LOCAL_IRQS	32

#define NR_MAX_VECTOR		0xFFU
#define VECTOR_INVALID		(NR_MAX_VECTOR + 1U)
#define NR_IRQS		1024

/* This is a spurious interrupt ID which never makes it into the GIC code. */
#define INVALID_IRQ     1023

extern const unsigned int nr_irqs;
#define nr_static_irqs NR_IRQS
#define arch_hwdom_irqs(domid) NR_IRQS

struct irq_desc *__irq_to_desc(int irq);

#define irq_to_desc(irq)    __irq_to_desc(irq)

void do_IRQ(struct cpu_regs *regs, unsigned int irq);

int release_guest_irq(struct acrn_vm *vm, unsigned int irq);

void init_IRQ(void);

int irq_set_type(unsigned int irq, unsigned int type);

void irq_set_affinity(struct irq_desc *desc, const cpumask_t *cpu_mask);

extern struct acrn_irqchip_ops *acrn_irqchip;

/*
 * Definition of the stack frame layout
 */
struct intr_excp_ctx {
	struct cpu_regs gp_regs;
	uint64_t vector;
	uint64_t error_code;
	uint64_t ip;
	uint64_t status;
	uint64_t sp;
};

/* RFLAGS */
#define HV_ARCH_VCPU_RFLAGS_IF              (1UL<<9U)
#define HV_ARCH_VCPU_RFLAGS_RF              (1UL<<16U)

/* Interruptability State info */

#define HV_ARCH_VCPU_BLOCKED_BY_NMI         (1UL<<3U)
#define HV_ARCH_VCPU_BLOCKED_BY_MOVSS       (1UL<<1U)
#define HV_ARCH_VCPU_BLOCKED_BY_STI         (1UL<<0U)

int32_t exception_vmexit_handler(struct acrn_vcpu *vcpu);
int32_t external_interrupt_vmexit_handler(struct acrn_vcpu *vcpu);
int32_t nmi_window_vmexit_handler(struct acrn_vcpu *vcpu);
int32_t interrupt_window_vmexit_handler(struct acrn_vcpu *vcpu);

/* vectors range for dynamic allocation, usually for devices */
#define VECTOR_DYNAMIC_START	0x20U
#define VECTOR_DYNAMIC_END	0xDFU
#define HYPERVISOR_CALLBACK_VHM_VECTOR	0xF3U

#endif /* __RISCV_IRQ_H__ */
