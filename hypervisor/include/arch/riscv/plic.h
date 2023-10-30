#ifndef __RISCV_PLIC_H__
#define __RISCV_PLIC_H__

#include <types.h>
#include <acrn/config.h>
#include <acrn/spinlock.h>
#include <public/irq.h>

#define PLIC_IPRR	(0x0000)
#define PLIC_IPER	(0x1000)
#define PLIC_IER	(0x2000)
#define PLIC_THR	(0x200000)
#define PLIC_EOIR	(0x200004)

#define PLIC_IRQ_MASK 	(0xFFFFFFFE)

struct acrn_plic {
	spinlock_t lock;
	paddr_t base;
	void __iomem * map_base;
	uint32_t size;
};

void plic_init(void);

#endif
