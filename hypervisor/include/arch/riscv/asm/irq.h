/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_IRQ_H__
#define __RISCV_IRQ_H__

#include <asm/cpu.h>
#include <asm/lib/spinlock.h>

#define NOTIFY_VCPU_SWI		0
#define SMP_FUNC_CALL		1

struct irq_guest
{
    struct acrn_vm *vm;
    uint32_t virq;
};

struct irq_desc;
struct acrn_irqchip_ops {
	const char *name;
	void (*init)(void);
	void (*set_irq_mask)(struct irq_desc *desc, uint32_t priority);
	void (*set_irq_priority)(struct irq_desc *desc, uint32_t priority);
	uint32_t (*get_irq)(void);
	void (*enable)(struct irq_desc *);
	void (*disable)(struct irq_desc *);
	void (*eoi)(struct irq_desc *desc);
};

extern struct acrn_irqchip_ops dummy_irqchip;

#define IRQ_INPROGRESS		(1 << 0)
#define IRQ_DISABLED		(1 << 1)
struct arch_irq_desc {
	uint32_t vector;
	uint32_t status;
	uint32_t type;
	struct irq_guest info;
	struct acrn_irqchip_ops *irqchip;
	uint64_t affinity;
};
#define irq_desc_initialized(desc) (((struct arch_irq_desc *)(desc)->arch_data)->irqchip != NULL)

#define NR_MAX_VECTOR		0xFFU
#define VECTOR_INVALID		(NR_MAX_VECTOR + 1U)

extern const uint32_t nr_irqs;
#define nr_static_irqs NR_IRQS

extern struct irq_desc *irq_to_desc(int irq);
extern void do_IRQ(struct cpu_regs *regs, uint32_t irq);
extern int irq_set_type(uint32_t irq, uint32_t type);

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

/* vectors range for dynamic allocation, usually for devices */
#define VECTOR_DYNAMIC_START	0x20U
#define VECTOR_DYNAMIC_END	0xDFU
#define HYPERVISOR_CALLBACK_HSM_VECTOR	0x20U

#define INVALID_INTERRUPT_PIN	0xffffffffU

extern bool request_irq_arch(uint32_t irq);
extern void free_irq_arch(uint32_t irq);
extern void pre_irq_arch(const struct irq_desc *desc);
extern void post_irq_arch(const struct irq_desc *desc);
extern uint32_t irq_to_vector(uint32_t irq);
extern void init_irq_descs_arch(struct irq_desc *descs);
extern void init_interrupt_arch(uint16_t pcpu_id);
extern void setup_irqs_arch(void);
extern void dispatch_interrupt(struct cpu_regs *regs);
extern void handle_mexti(void);

#endif /* __RISCV_IRQ_H__ */
