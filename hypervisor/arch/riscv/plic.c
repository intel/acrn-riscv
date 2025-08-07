/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <asm/irq.h>
#include <debug/logmsg.h>
#include <asm/init.h>
#include <asm/mem.h>
#include <asm/pgtable.h>
#include <asm/plic.h>
#include <asm/lib/spinlock.h>
#include <asm/lib/bits.h>
#include <asm/io.h>

struct acrn_plic phy_plic;
struct acrn_plic *plic = &phy_plic;

void plic_write8(struct acrn_plic *plic, uint32_t value, uint32_t offset)
{
	mmio_writeb(value, plic->map_base + offset);
}

void plic_write32(uint32_t value, uint32_t offset)
{
	mmio_writel(value, plic->map_base + offset);
}

uint32_t plic_read32(uint32_t offset)
{
	return mmio_readl(plic->map_base + offset);
}

static void plic_set_irq(struct irq_desc *irqd, uint32_t offset)
{
	uint32_t base, val;

	base = offset + (irqd->irq / 32) * 4;
	val = plic_read32(base);
	val |= 1U << (irqd->irq % 32);
	// 1bits/IRQ
	plic_write32(val, base);
}

static void plic_clear_irq(struct irq_desc *irqd, uint32_t offset)
{
	uint32_t base, val;

	base = offset + (irqd->irq / 32) * 4;
	val = plic_read32(base);
	val &= ~(1U << (irqd->irq % 32));
	// 1bits/IRQ
	plic_write32(val, base);
}

void plic_set_address(void)
{
	plic->base = CONFIG_PLIC_BASE;
	plic->size = CONFIG_PLIC_SIZE;
}

void plic_init_map(void){
	plic->map_base = hpa2hva(plic->base);
}

static void plic_set_irq_mask(struct irq_desc *desc, uint32_t priority)
{
	uint64_t flags;

	spin_lock_irqsave(&plic->lock, &flags);
	plic_write32(priority & 0x7, PLIC_THR);
	spin_unlock_irqrestore(&plic->lock, flags);
}

static void plic_set_irq_priority(struct irq_desc *desc, uint32_t priority)
{
	uint32_t irq = desc->irq;
	uint64_t flags;

	spin_lock_irqsave(&plic->lock, &flags);
	plic_write32(priority & 0x7, PLIC_IPRR + irq * 4);
	spin_unlock_irqrestore(&plic->lock, flags);
}

static void plic_irq_enable(struct irq_desc *desc)
{
	uint64_t flags;

	spin_lock_irqsave(&plic->lock, &flags);
	plic_set_irq(desc, PLIC_IER);
	clear_bit(IRQ_DISABLED, &((struct arch_irq_desc *)desc->arch_data)->status);
	dsb();
	spin_unlock_irqrestore(&plic->lock, flags);
}

static void plic_irq_disable(struct irq_desc *desc)
{
	uint64_t flags;

	spin_lock_irqsave(&plic->lock, &flags);
	plic_clear_irq(desc, PLIC_IER);
	set_bit(IRQ_DISABLED, &((struct arch_irq_desc *)desc->arch_data)->status);
	dsb();
	spin_unlock_irqrestore(&plic->lock, flags);
}

static uint32_t plic_get_irq(void)
{
	return plic_read32(PLIC_EOIR);
}

static void plic_eoi_irq(struct irq_desc *desc)
{
	plic_write32(desc->irq, PLIC_EOIR);
}

struct acrn_irqchip_ops plic_ops = {
	.name     		= "sifive-plic",
	.init			= plic_init,
	.set_irq_mask 		= plic_set_irq_mask,
	.set_irq_priority 	= plic_set_irq_priority,
	.get_irq 		= plic_get_irq,
	.enable       		= plic_irq_enable,
	.disable      		= plic_irq_disable,
	.eoi			= plic_eoi_irq,
};

void plic_init(void)
{
	acrn_irqchip = &plic_ops;
	plic_set_address();
	pr_info("plic base: %lx size: %lx", plic->base, plic->size);
	spinlock_init(&plic->lock);
	spin_lock(&plic->lock);
	plic_init_map();
	spin_unlock(&plic->lock);
}
