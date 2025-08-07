/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#define pr_prefix		"vclint: "

#include <types.h>
#include <errno.h>
#include <timer.h>
#include <asm/lib/bits.h>
#include <asm/lib/atomic.h>
#include <asm/per_cpu.h>
#include <asm/pgtable.h>
#include <asm/apicreg.h>
#include <asm/irq.h>
#include <asm/guest/vmcs.h>
#include <asm/guest/vclint.h>
#include <asm/vmx.h>
#include <asm/guest/vcsr.h>
#include <asm/guest/vm.h>
#include <asm/guest/s2vm.h>
#include <asm/guest/virq.h>
#include <asm/guest/instr_emul.h>
#include <trace.h>
#include <logmsg.h>
#include "vclint_priv.h"

#define VCLINT_VERBOS 0

static inline uint32_t prio(uint32_t x)
{
	return (x >> 4U);
}

#define DBG_LEVEL_VCLINT	6U

static inline struct acrn_vcpu *vector2vcpu(const uint32_t vector)
{
	return NULL;
}

#if VCLINT_VERBOS
static inline void vclint_dump_msip(const struct acrn_vclint *vclint, const char *msg)
{
	const struct clint_reg *irrptr = &(vclint->clint_page.irr[0]);

	for (uint8_t i = 0U; i < 8U; i++) {
		dev_dbg(DBG_LEVEL_VCLINT, "%s irr%u 0x%08x", msg, i, irrptr[i].v);
	}
}

static inline void vclint_dump_isr(const struct acrn_vclint *vclint, const char *msg)
{
	const struct clint_reg *isrptr = &(vclint->clint_page.isr[0]);

	for (uint8_t i = 0U; i < 8U; i++) {
		dev_dbg(DBG_LEVEL_VCLINT, "%s isr%u 0x%08x", msg, i, isrptr[i].v);
	}
}
#else
static inline void vclint_dump_irr(__unused const struct acrn_vclint *vclint, __unused const char *msg) {}

static inline void vclint_dump_isr(__unused const struct acrn_vclint *vclint, __unused const char *msg) {}
#endif

/*
 * Post an interrupt to the vcpu running on 'hostcpu'. This will use a
 * hardware assist if available (e.g. Posted Interrupt) or fall back to
 * sending an 'ipinum' to interrupt the 'hostcpu'.
 */
static void vclint_timer_expired(void *data);

static inline bool vclint_enabled(const struct acrn_vclint *vclint)
{
	return true;
}

/**
 * @pre vclint != NULL
 */
static void vclint_init_timer(struct acrn_vclint *vclint, uint32_t idx)
{
	struct vclint_timer *vtimer;
	struct acrn_vcpu *vcpu;

	vcpu = vcpu_from_vid(vclint->vm, idx);

	vtimer = &vclint->vtimer[idx];
	(void)memset(vtimer, 0U, sizeof(struct vclint_timer));

	initialize_timer(&vtimer->timer,
			vclint_timer_expired, vcpu,
			0UL, 0UL);
}

/**
 * @pre vclint != NULL
 */
static void vclint_reset_timer(struct acrn_vclint *vclint)
{
	struct hv_timer *timer;

	for (int i = 0; i < 5; i++) {
		timer = &vclint->vtimer[i].timer;
		del_timer(timer);
		timer->mode = TICK_MODE_ONESHOT;
		timer->timeout = 0UL;
		timer->period_in_cycle = 0UL;
	}
}

void vclint_write_tmr(struct acrn_vclint *vclint, uint32_t index, uint64_t data)
{
	struct hv_timer *timer = &vclint->vtimer[index].timer;

	del_timer(timer);
	timer->mode = TICK_MODE_ONESHOT;
	timer->timeout = data;
	timer->period_in_cycle = 0UL;
	(void)add_timer(timer);
}

uint64_t vclint_get_tsc_deadline_csr(const struct acrn_vclint *vclint)
{
	/*
	 * FIXME: AndreiW.
	 */
	return 0;
}

void vclint_set_tsc_deadline_csr(struct acrn_vclint *vclint, uint32_t index, uint64_t val_arg)
{
	struct hv_timer *timer;
	uint64_t val = val_arg;

	timer = &vclint->vtimer[index].timer;
	del_timer(timer);

	if (val != 0UL) {
		/* transfer guest tsc to host tsc */
		timer->timeout = val;
		/* vclint_init_timer has been called,
		 * and timer->timeout is not 0,here
		 * add_timer should not return error
		 */
		(void)add_timer(timer);
	} else {
		timer->timeout = 0UL;
	}
}

static void
vclint_write_msip(struct acrn_vclint *vclint, uint32_t idx, uint32_t data)
{
	struct clint_regs *clint;
	struct acrn_vcpu *vcpu = &vclint->vm->hw.vcpu[idx];
	uint64_t flags;

	spin_lock_irqsave(&vclint->lock, &flags);
	clint = &(vclint->clint_page);
//	if (clint->msip[idx] & 0x1)
//		return;
	clint->msip[idx] = data & 0x1;
	vclint_set_intr(vcpu);
	spin_unlock_irqrestore(&vclint->lock, flags);
}

static int32_t vclint_read(struct acrn_vclint *vclint, uint32_t offset_arg, uint64_t *data)
{
	int32_t ret = 0;
	struct clint_regs *clint = &(vclint->clint_page);
	uint64_t offset = offset_arg - CLINT_MEM_ADDR;
	*data = 0UL;

	if (offset > sizeof(*clint)) {
		ret = -EACCES;
	} else {
		switch (offset) {
		case CLINT_OFFSET_TIMER0:
		case CLINT_OFFSET_TIMER1:
		case CLINT_OFFSET_TIMER2:
		case CLINT_OFFSET_TIMER3:
		case CLINT_OFFSET_TIMER4:
			*data = clint->mtimer[(offset - CLINT_OFFSET_TIMER0) >> 3];
			break;
		case CLINT_OFFSET_MSIP0:
		case CLINT_OFFSET_MSIP1:
		case CLINT_OFFSET_MSIP2:
		case CLINT_OFFSET_MSIP3:
		case CLINT_OFFSET_MSIP4:
			*data = clint->msip[(offset - CLINT_OFFSET_MSIP0) >> 2];
			break;
		case CLINT_OFFSET_MTIME:
			*data = clint->mtime;
			pr_info("vclint mtime %x\n", *data);
			break;
		default:
			ret = -EACCES;
			break;
		}
	}

	return ret;
}

static int32_t vclint_write(struct acrn_vclint *vclint, uint64_t offset, uint64_t data)
{
	struct clint_regs *clint = &(vclint->clint_page);
	uint32_t data32 = (uint32_t)data;
	int32_t ret = 0;

	offset -= vclint->clint_base;
	if (offset <= sizeof(*clint)) {
		switch (offset) {
		case CLINT_OFFSET_MSIP0:
		case CLINT_OFFSET_MSIP1:
		case CLINT_OFFSET_MSIP2:
		case CLINT_OFFSET_MSIP3:
		case CLINT_OFFSET_MSIP4:
			vclint_write_msip(vclint, (offset - CLINT_OFFSET_MSIP0) >> 2, data32);
			break;
		case CLINT_OFFSET_TIMER0:
		case CLINT_OFFSET_TIMER1:
		case CLINT_OFFSET_TIMER2:
		case CLINT_OFFSET_TIMER3:
		case CLINT_OFFSET_TIMER4:
			vclint_write_tmr(vclint, (offset - CLINT_OFFSET_TIMER0) >> 3, data);
			break;
		case CLINT_OFFSET_MTIME:
			clint->mtime = data;
			for (int i = 0; i < 5; i++)
				del_timer(&vclint->vtimer[i].timer);
			pr_info("vclint mtime %x\n", data);
			break;
		default:
			ret = -EACCES;
			/* Read only */
			break;
		}
	} else {
		ret = -EACCES;
	}

	return ret;
}

void vclint_send_ipi(struct acrn_vclint *vclint, uint32_t cpu)
{
	uint64_t offset;
	uint64_t data;

	offset = vclint->clint_base + cpu * 4;
	data = 0x1UL;
	vclint_write(vclint, offset, data);

	return;
}

/*
 * @pre vclint != NULL && ops != NULL
 */
void
vclint_reset(struct acrn_vclint *vclint, const struct acrn_vclint_ops *ops, enum reset_mode mode)
{
	struct clint_regs *clint;

	vclint->clint_base = DEFAULT_CLINT_BASE;

	if (mode == INIT_RESET) {
		vclint->clint_base = DEFAULT_CLINT_BASE;
	}
	clint = &(vclint->clint_page);
	(void)memset((void *)clint, 0U, sizeof(struct clint_regs));

	vclint_reset_timer(vclint);
	vclint->ops = ops;
}

uint64_t vclint_get_clintbase(const struct acrn_vclint *vclint)
{
	return vclint->clint_base;
}

int32_t vclint_set_clintbase(struct acrn_vclint *vclint, uint64_t new)
{
	int32_t ret = 0;

	if (vclint->clint_base != new) {
		vclint->clint_base = new;

		/*
		 * TODO: Logic to check for change in Reserved Bits and Inject GP
		 */

	}

	return ret;
}

void vclint_set_intr(struct acrn_vcpu *vcpu)
{
	signal_event(&(vcpu->events[VCPU_EVENT_VIRTUAL_INTERRUPT]));
	vcpu_make_request(vcpu, ACRN_REQUEST_EVENT);
}

/* interrupt context */
static void vclint_timer_expired(void *data)
{
	struct acrn_vcpu *vcpu = data;
	struct acrn_vclint *vclint = vcpu_vclint(vcpu);
	uint64_t flags;

	spin_lock_irqsave(&vclint->lock, &flags);
	set_bit(vcpu->vcpu_id, &vclint->mtip);
	vclint_set_intr(vcpu);
	spin_unlock_irqrestore(&vclint->lock, flags);
}

/*
 *  @pre vcpu != NULL
 */
void vclint_free(struct acrn_vcpu *vcpu)
{
	struct acrn_vclint *vclint = vcpu_vclint(vcpu);

	for (int i = 0; i < 5; i++)
		del_timer(&vclint->vtimer[i].timer);
}

/**
 *CLINT-v: Get the HPA to CLINT-access page
 * **/
uint64_t
vclint_get_clint_access_addr(void)
{
	/*CLINT-v CLINT-access address */
	static uint8_t apicv_apic_access_addr[PAGE_SIZE] __aligned(PAGE_SIZE);

	return hva2hpa(apicv_apic_access_addr);
}

/**
 *CLINT-v: Get the HPA to virtualized CLINT registers page
 * **/
uint64_t
vclint_get_clint_page_addr(struct acrn_vclint *vclint)
{
	return hva2hpa(&(vclint->clint_page));
}

bool vclint_find_deliverable_intr(const struct acrn_vcpu *vcpu, uint32_t *vector)
{
	struct acrn_vclint *vclint = vcpu_vclint(vcpu);
	bool ret = false;
	uint64_t flags;

	spin_lock_irqsave(&vclint->lock, &flags);
	if ((vclint->clint_page.msip[vcpu->vcpu_id] & 0x1)) {
		*vector |= CLINT_VECTOR_SSI;
		vclint->clint_page.msip[vcpu->vcpu_id] &= ~0x1;
		ret = true;
	}
	if (test_bit(vcpu->vcpu_id, vclint->mtip)) {
		*vector |= CLINT_VECTOR_STI;
		clear_bit(vcpu->vcpu_id, &vclint->mtip);
		ret = true;
	}
	spin_unlock_irqrestore(&vclint->lock, flags);

	return ret;
}

void vcpu_inject_intr(struct acrn_vcpu *vcpu)
{
	struct guest_cpu_context *ctx = &vcpu->arch.contexts[vcpu->arch.cur_context];
	uint32_t vector = 0U;

	if (vclint_find_deliverable_intr(vcpu, &vector))
		ctx->run_ctx.sip |= vector;
}

bool vclint_has_pending_intr(struct acrn_vcpu *vcpu)
{
	struct acrn_vclint *vclint = vcpu_vclint(vcpu);

	return vclint->clint_page.msip[vcpu->vcpu_id] & 0x1;
}

static bool vclint_has_pending_delivery_intr(struct acrn_vcpu *vcpu)
{
	if (vclint_has_pending_intr(vcpu))
		vcpu_make_request(vcpu, ACRN_REQUEST_EVENT);

	return vcpu->arch.pending_req != 0UL;
}

static bool vclint_clint_read_access_may_valid(__unused uint32_t offset)
{
	return true;
}

static bool vclint_clint_write_access_may_valid(__unused uint32_t offset)
{
	return true;
}

int32_t vclint_access_handler(struct io_request *io_req, void *private_data)
{
	struct acrn_vclint *vclint = (struct acrn_vclint *)private_data;
	struct acrn_mmio_request *mmio = &io_req->reqs.mmio_request;
	uint32_t offset = mmio->address;
	int32_t ret = 0;

	if (mmio->direction == ACRN_IOREQ_DIR_WRITE) {
		if (vclint->ops->clint_write_access_may_valid(offset))
			ret = vclint_write(vclint, offset, mmio->value);
	} else {
		if (vclint->ops->clint_read_access_may_valid(offset))
			ret = vclint_read(vclint, offset, &mmio->value);
		else
			mmio->value = 0UL;
	}

	return ret;
}

static const struct acrn_vclint_ops acrn_vclint_ops = {
	.has_pending_delivery_intr = vclint_has_pending_delivery_intr,
	.has_pending_intr = vclint_has_pending_intr,
	.clint_read_access_may_valid = vclint_clint_read_access_may_valid,
	.clint_write_access_may_valid = vclint_clint_write_access_may_valid,
};

/**
 *  @pre vm != NULL
 */
void vclint_init(struct acrn_vm *vm)
{
	struct acrn_vclint *vclint = &vm->vclint;

	/* only need unmap it from SOS as UOS never mapped it */
#if 0
	uint64_t *pml4_page = (uint64_t *)vm->arch_vm.s2ptp;
	if (is_sos_vm(vm)) {
		s2pt_del_mr(vm, pml4_page,
			DEFAULT_CLINT_BASE, DEFAULT_CLINT_SIZE);
	}
	s2pt_add_mr(vm, pml4_page,
		vclint_get_clint_access_addr(),
		DEFAULT_CLINT_BASE, PAGE_SIZE,
		PAGE_U | PAGE_ATTR_IO);
#endif

	spinlock_init(&vclint->lock);
	vclint->vm = vm;
	vclint->clint_base = DEFAULT_CLINT_BASE;
	vclint->ops = &acrn_vclint_ops;
	vclint->clint_page.mtime = (uint64_t)get_tick();
	for (int i = 0; i < VCLINT_LVT_MAX; i++)
		vclint_init_timer(vclint, i);

	register_mmio_emulation_handler(vm, vclint_access_handler, (uint64_t)vclint->clint_base,
		(uint64_t)vclint->clint_base + DEFAULT_CLINT_SIZE, (void *)vclint, false);
}

const struct acrn_vclint_ops *vclint_ops = &acrn_vclint_ops;
