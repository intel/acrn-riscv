/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 *   Haibo1 Xu <haibo1.xu@intel.com>
 */

#define pr_prefix		"vplic: "

#include <types.h>
#include <errno.h>
#include <asm/pgtable.h>
#include <asm/irq.h>
#include <asm/plic.h>
#include <asm/guest/instr_emul.h>
#include <asm/guest/virq.h>
#include <asm/guest/vmcs.h>
#include <asm/guest/vplic.h>
#include <asm/vmx.h>
#include <asm/guest/vm.h>
#include <ptdev.h>
#include <trace.h>
#include <logmsg.h>
#include "vplic_priv.h"

#define VPLIC_VERBOS	0
#define DBG_LEVEL_VPLIC	6U

#if VPLIC_VERBOS
static inline void vplic_dump_regs(const struct acrn_vplic *vplic)
{
	const struct plic_regs *regs = &vplic->regs;

	dev_dbg(DBG_LEVEL_VPLIC, "VPLIC Source Priority Reg:\n");
	for (int i = 0; i < PLIC_NUM_SOURCES; i++) {
		dev_dbg(DBG_LEVEL_VPLIC, "0x%04x, ", regs->source_priority[i]);
	}

	dev_dbg(DBG_LEVEL_VPLIC, "VPLIC Pending Reg:\n");
	for (int i = 0; i < PLIC_NUM_FIELDS; i++) {
		dev_dbg(DBG_LEVEL_VPLIC, "0x%04x, ", regs->pending[i]);
	}

	dev_dbg(DBG_LEVEL_VPLIC, "VPLIC Enable Reg:\n");
	for (int i = 0; i < PLIC_NUM_CONTEXT; i++) {
		dev_dbg(DBG_LEVEL_VPLIC, "Contex %d:\n", i);
                for (int j = 0; j < PLIC_NUM_FIELDS; j++) {
			dev_dbg(DBG_LEVEL_VPLIC, "0x%04x, ", regs->enable[i][j]);
		}
	}

	dev_dbg(DBG_LEVEL_VPLIC, "VPLIC Target Priority Reg:\n");
	for (int i = 0; i < PLIC_NUM_CONTEXT; i++) {
		dev_dbg(DBG_LEVEL_VPLIC, "0x%04x, ", regs->target_priority[i]);
	}

	dev_dbg(DBG_LEVEL_VPLIC, "VPLIC Claimed Reg:\n");
	for (int i = 0; i < PLIC_NUM_FIELDS; i++) {
		dev_dbg(DBG_LEVEL_VPLIC, "0x%04x, ", regs->claimed[i]);
	}
}
#else
static inline void vplic_dump_regs(__unused const struct acrn_vplic *vplic) {}
#endif

const struct acrn_apicv_ops *vplic_ops;

static void vplic_reg_set_bit(volatile void *p, uint32_t nr)
{
	*(uint32_t *)p |= (1 << nr);
}

static void vplic_reg_clear_bit(volatile void *p, uint32_t nr)
{
	*(uint32_t *)p &= ~(1 << nr);
}

static void vplic_set_pending(struct plic_regs *regs, uint32_t irq)
{
        vplic_reg_set_bit(&regs->pending[irq >> 5], irq & 31);
}

static void vplic_clear_pending(struct plic_regs *regs, uint32_t irq)
{
        vplic_reg_clear_bit(&regs->pending[irq >> 5], irq & 31);
}

static void vplic_set_claimed(struct plic_regs *regs, uint32_t irq)
{
        vplic_reg_set_bit(&regs->claimed[irq >> 5], irq & 31);
}

static void vplic_clear_claimed(struct plic_regs *regs, uint32_t irq)
{
        vplic_reg_clear_bit(&regs->claimed[irq >> 5], irq & 31);
}

static uint32_t vplic_get_deliverable_irq(struct acrn_vplic *vplic, uint32_t context_id)
{
	uint32_t max_irq = 0;
	struct plic_regs *regs = &vplic->regs;
	uint32_t max_prio = regs->target_priority[context_id];

	for (int i = 0; i < PLIC_NUM_FIELDS; i++) {
		uint32_t pending_enabled_not_claimed = (regs->pending[i] & ~regs->claimed[i]) &
							regs->enable[context_id][i];

		if (!pending_enabled_not_claimed)
			continue;

		for (int j = 0; j < 32; j++) {
			uint32_t irq = (i << 5) + j;
			uint32_t prio = regs->source_priority[irq];
			bool enabled = !!(pending_enabled_not_claimed & (1 << j));

			if (enabled && prio > max_prio) {
				max_irq = irq;
				max_prio = prio;
			}
		}
	}

	return max_irq;
}

static void vplic_set_intr(struct acrn_vcpu *vcpu)
{
        signal_event(&(vcpu->events[VCPU_EVENT_VIRTUAL_INTERRUPT]));
        vcpu_make_request(vcpu, ACRN_REQUEST_EXTINT);
}

static void vplic_update(struct acrn_vplic *vplic)
{
        for (uint32_t context_id = 0; context_id < vplic->vm->hw.created_vcpus; context_id++) {
		struct acrn_vcpu *vcpu = &vplic->vm->hw.vcpu[context_id];
		vplic_set_intr(vcpu);
	}
}

static bool offset_between(uint32_t offset, uint32_t base, uint32_t num)
{
	return offset >= base && offset - base < num;
}

static int32_t vplic_read(struct acrn_vplic *vplic, uint32_t offset, uint64_t *data)
{
	int32_t ret = 0;
	struct plic_regs *regs = &vplic->regs;
	uint64_t flags;

	*data = 0UL;

	spin_lock_irqsave(&vplic->lock, &flags);
	if (offset_between(offset, vplic->priority_base, PLIC_NUM_SOURCES << 2)) {
		uint32_t src_index = (offset - vplic->priority_base) >> 2;

		*data = regs->source_priority[src_index];
	} else if (offset_between(offset, vplic->pending_base, (PLIC_NUM_SOURCES + 31) >> 3)) {
		uint32_t word_index = (offset - vplic->pending_base) >> 2;

		*data = regs->pending[word_index];
	} else if (offset_between(offset, vplic->enable_base,
				  PLIC_NUM_CONTEXT * PLIC_ENABLE_STRIDE)) {
		uint32_t context_index = (offset - vplic->enable_base) / PLIC_ENABLE_STRIDE;
		uint32_t word_index = (offset & (PLIC_ENABLE_STRIDE - 1)) >> 2;

		if (word_index < PLIC_NUM_FIELDS)
			*data = regs->enable[context_index][word_index];
	} else if (offset_between(offset, vplic->dst_prio_base,
				  PLIC_NUM_CONTEXT * PLIC_DST_PRIO_STRIDE)) {
		uint32_t context_index = (offset - vplic->dst_prio_base) / PLIC_DST_PRIO_STRIDE;
		uint32_t reg_id = (offset & (PLIC_DST_PRIO_STRIDE - 1));

		if (reg_id == 0) { // Target priority threshold register
			*data = regs->target_priority[context_index];
		} else if (reg_id == 4) { // Claim/complete register
			uint32_t irq = 0;

			irq = vplic_get_deliverable_irq(vplic, context_index);
			if (irq) {
				vplic_clear_pending(regs, irq);
				vplic_set_claimed(regs, irq);
			}
			vplic_update(vplic);

			*data = irq;
		} else {
			dev_dbg(DBG_LEVEL_VPLIC, "vplic read: invalid  context reg id %x\n", reg_id);
			ret = -EACCES;
		}
	} else {
		dev_dbg(DBG_LEVEL_VPLIC, "vplic read: invalid offset %x\n", offset);
		ret = -EACCES;
	}
	spin_unlock_irqrestore(&vplic->lock, flags);

	dev_dbg(DBG_LEVEL_VPLIC, "vplic read offset %x, data %lx\n", offset, *data);
	vplic_dump_regs(vplic);

	return ret;
}

static int32_t vplic_write(struct acrn_vplic *vplic, uint32_t offset, uint64_t data)
{
	int32_t ret = 0;
	struct plic_regs *regs = &vplic->regs;
	uint64_t flags;

	dev_dbg(DBG_LEVEL_VPLIC, "vplic write offset %#x, data %#lx", offset, data);

	spin_lock_irqsave(&vplic->lock, &flags);
	if (offset_between(offset, vplic->priority_base, PLIC_NUM_SOURCES << 2)) {
		uint32_t src_index = (offset - vplic->priority_base) >> 2;

                if (data <= PLIC_NUM_PRIORITY) {
			regs->source_priority[src_index] = data;
                        vplic_update(vplic);
                } else {
			dev_dbg(DBG_LEVEL_VPLIC, "vplic write: invalid source priority value %x\n", data);
		}

		if (is_service_vm(vplic->vm))
			plic_write32(data, offset);
	} else if (offset_between(offset, vplic->pending_base, (PLIC_NUM_SOURCES + 31) >> 3)) {
		dev_dbg(DBG_LEVEL_VPLIC, "vplic write: invalid pending reg write %x\n", offset);
	} else if (offset_between(offset, vplic->enable_base,
				  PLIC_NUM_CONTEXT * PLIC_ENABLE_STRIDE)) {
		uint32_t context_index = (offset - vplic->enable_base) / PLIC_ENABLE_STRIDE;
		uint32_t word_index = (offset & (PLIC_ENABLE_STRIDE - 1)) >> 2;

		if (word_index < PLIC_NUM_FIELDS)
			regs->enable[context_index][word_index] = data;
		else
			dev_dbg(DBG_LEVEL_VPLIC, "vplic write: invalid enable reg write %x\n", offset);

		if (is_service_vm(vplic->vm))
			plic_write32(data, PLIC_IER + (word_index << 2)); // Deliver phy irq to context 0
	} else if (offset_between(offset, vplic->dst_prio_base,
				  PLIC_NUM_CONTEXT * PLIC_DST_PRIO_STRIDE)) {
		uint32_t context_index = (offset - vplic->dst_prio_base) / PLIC_DST_PRIO_STRIDE;
		uint32_t reg_id = (offset & (PLIC_DST_PRIO_STRIDE - 1));

		if (reg_id == 0) { // Target priority threshold register
			if (data <= PLIC_NUM_PRIORITY) {
				regs->target_priority[context_index] = data;
				vplic_update(vplic);
			}

			if (is_service_vm(vplic->vm))
				plic_write32(data, PLIC_THR);
		} else if (reg_id == 4) { // Claim/complete register
			if (data < PLIC_NUM_SOURCES) {
				// Update the claimed reg
				vplic_clear_claimed(regs, data);
				vplic_update(vplic);
			}

			if (is_service_vm(vplic->vm))
				plic_write32(data, PLIC_EOIR);
		} else {
			dev_dbg(DBG_LEVEL_VPLIC, "vplic write: invalid  context reg id %x\n", reg_id);
			ret = -EACCES;
		}
	} else {
		dev_dbg(DBG_LEVEL_VPLIC, "vplic write: invalid offset %x\n", offset);
		ret = -EACCES;
	}
	spin_unlock_irqrestore(&vplic->lock, flags);

	vplic_dump_regs(vplic);

	return ret;
}

/*
 * @pre vplic != NULL && ops != NULL
 */
void
vplic_reset(struct acrn_vplic *vplic, const struct acrn_vplic_ops *ops, enum reset_mode mode)
{
        struct plic_regs *regs;

        if (mode == INIT_RESET) {
                vplic->plic_base = DEFAULT_PLIC_BASE;
        }

        regs = &(vplic->regs);
        memset((void *)regs, 0U, sizeof(struct plic_regs));

        vplic->ops = ops;
}

void vplic_accept_intr(struct acrn_vcpu *vcpu, uint32_t vector, bool level)
{
	struct acrn_vplic *vplic;
	uint64_t flags;

	vplic = vcpu_vplic(vcpu);
	spin_lock_irqsave(&vplic->lock, &flags);
	if (vector < PLIC_NUM_SOURCES) {
		if (level)
			vplic_set_pending(&vplic->regs, vector);
		else
			vplic_clear_pending(&vplic->regs, vector);

		vplic_update(vplic);
	} else {
		dev_dbg(DBG_LEVEL_VPLIC, "vplic ignoring interrupt to vector %u", vector);
	}
	spin_unlock_irqrestore(&vplic->lock, flags);
}

void vcpu_inject_extint(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];
	struct acrn_vplic *vplic = vcpu_vplic(vcpu);
	uint32_t irq;
#ifdef CONFIG_MACRN
	uint64_t value = cpu_csr_read(mip);
#endif
	uint64_t flags;

	spin_lock_irqsave(&vplic->lock, &flags);
	irq = vplic_get_deliverable_irq(vplic, vcpu->vcpu_id);
	if (irq) {
		ctx->run_ctx.sip |= CLINT_VECTOR_SEI;
#ifdef CONFIG_MACRN
		cpu_csr_write(mip, value | CLINT_VECTOR_SEI);
#endif
	} else {
		ctx->run_ctx.sip &= ~CLINT_VECTOR_SEI;
#ifdef CONFIG_MACRN
		cpu_csr_write(mip, value & ~CLINT_VECTOR_SEI);
#endif
	}
	spin_unlock_irqrestore(&vplic->lock, flags);
}

static bool vplic_read_access_may_valid(__unused uint32_t offset)
{
	return true;
}

static bool vplic_write_access_may_valid(__unused uint32_t offset)
{
	return true;
}

int32_t vplic_access_handler(struct io_request *io_req, void *private_data)
{
	struct acrn_vplic *vplic = (struct acrn_vplic *)private_data;
	struct acrn_mmio_request *mmio = &io_req->reqs.mmio_request;
	uint32_t offset = mmio->address - vplic->plic_base;
	int32_t ret = 0;

	if (mmio->size == 4) {
		if (mmio->direction == ACRN_IOREQ_DIR_READ)
			ret = vplic_read(vplic, offset, &mmio->value);
		else
			ret = vplic_write(vplic, offset, mmio->value);
	} else {
		pr_err("All RW to PLIC must be 32-bits in size");
		ret = -EINVAL;
	}

	return ret;
}

static const struct acrn_vplic_ops acrn_vplic_ops = {
	.plic_read_access_may_valid = vplic_read_access_may_valid,
	.plic_write_access_may_valid = vplic_write_access_may_valid,
};

void vplic_init(struct acrn_vm *vm)
{
	struct acrn_vplic *vplic = &vm->vplic;

	spinlock_init(&vplic->lock);
	vplic->vm = vm;
	vplic->plic_base = DEFAULT_PLIC_BASE;
	vplic->ops = &acrn_vplic_ops;

	vplic->priority_base = PLIC_SRC_PRIORITY_BASE;
	vplic->pending_base = PLIC_PENDING_BASE;
	vplic->enable_base = PLIC_ENABLE_BASE;
	vplic->dst_prio_base = PLIC_DST_PRIO_BASE;

	register_mmio_emulation_handler(vm, vplic_access_handler, (uint64_t)vplic->plic_base,
		(uint64_t)vplic->plic_base + DEFAULT_PLIC_SIZE, (void *)vplic, false);

	memset(&vplic->regs, 0U, sizeof(struct plic_regs));
}
