/*-
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 * Copyright (c) 2023 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NETAPP, INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NETAPP, INC OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#define pr_prefix		"vclint: "

#include <types.h>
#include <errno.h>
#include <acrn/bitops.h>
#include <asm/atomic.h>
#include <acrn/percpu.h>
#include <asm/pgtable.h>
#include <asm/csr.h>
#include <asm/irq.h>
#include <asm/guest/vmcs.h>
#include <asm/guest/vclint.h>
#include <asm/vmx.h>
#include <asm/guest/vm.h>
#include <asm/guest/s2vm.h>
#include <ptdev.h>
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

const struct acrn_apicv_ops *vclint_ops;

static bool apicv_set_intr_ready(struct acrn_vclint *vclint, uint32_t vector);

static void vclint_timer_expired(void *data);

static inline bool vclint_enabled(const struct acrn_vclint *vclint)
{
	return true;
}

static struct acrn_vclint *
vm_clint_from_vcpu_id(struct acrn_vm *vm, uint16_t vcpu_id)
{
	struct acrn_vcpu *vcpu;

	vcpu = vcpu_from_vid(vm, vcpu_id);

	return vcpu_vclint(vcpu);
}

static inline uint32_t
vclint_timer_divisor_shift(uint32_t dcr)
{
	uint32_t val;

	val = ((dcr & 0x3U) | ((dcr & 0x8U) >> 1U));
	return ((val + 1U) & 0x7U);
}

static inline bool
vclint_lvtt_tsc_deadline(const struct acrn_vclint *vclint)
{
	return true;
}

static inline bool
vclint_lvtt_masked(const struct acrn_vclint *vclint)
{
	return false;
}

static void vclint_init_timer(struct acrn_vclint *vclint)
{
	struct vclint_timer *vtimer;

	vtimer = &vclint->vtimer;
	(void)memset(vtimer, 0U, sizeof(struct vclint_timer));

	initialize_timer(&vtimer->timer,
			vclint_timer_expired, NULL,
			0UL, 0, 0UL);
}

static void vclint_reset_timer(struct acrn_vclint *vclint)
{
	struct hv_timer *timer;

	timer = &vclint->vtimer.timer;
	del_timer(timer);
	timer->mode = TICK_MODE_ONESHOT;
	timer->fire_tick = 0UL;
	timer->period_in_cycle = 0UL;
}

static bool
set_expiration(struct acrn_vclint *vclint)
{
	uint64_t now = rdtsc();
	uint64_t delta;
	struct vclint_timer *vtimer;
	struct hv_timer *timer;
	uint32_t tmicr, divisor_shift;
	bool ret;

	vtimer = &vclint->vtimer;
	tmicr = vtimer->tmicr;
	divisor_shift = vtimer->divisor_shift;

	if ((tmicr == 0U) || (divisor_shift > 8U)) {
		ret = false;
	} else {
		delta = (uint64_t)tmicr << divisor_shift;
		timer = &vtimer->timer;
		timer->fire_tick = now + delta;
		ret = true;
	}
	return ret;
}

static void vclint_update_lvtt(struct acrn_vclint *vclint,
			uint32_t val)
{
	uint32_t timer_mode = val & CLINT_LVTT_TM;
	struct vclint_timer *vtimer = &vclint->vtimer;

	if (vtimer->mode != timer_mode) {
		struct hv_timer *timer = &vtimer->timer;

		del_timer(timer);
		timer->mode = (timer_mode == CLINT_LVTT_TM_PERIODIC) ?
				TICK_MODE_PERIODIC: TICK_MODE_ONESHOT;
		timer->fire_tick = 0UL;
		timer->period_in_cycle = 0UL;

		vtimer->mode = timer_mode;
	}
}

static void vclint_write_tmr(struct acrn_vclint *vclint)
{
	struct clint_regs *clint;
	struct vclint_timer *vtimer;

	if (!vclint_lvtt_tsc_deadline(vclint)) {
		clint = &(vclint->clint_page);
		vtimer = &vclint->vtimer;
		vtimer->tmicr = clint->mtimer0;

		del_timer(&vtimer->timer);
		if (set_expiration(vclint)) {
			(void)add_timer(&vtimer->timer);
		}
	}
}

uint64_t vclint_get_tsc_deadline_csr(const struct acrn_vclint *vclint)
{
	uint64_t ret;
	return ret;
}

void vclint_set_tsc_deadline_csr(struct acrn_vclint *vclint, uint64_t val_arg)
{
	struct hv_timer *timer;
	uint64_t val = val_arg;

	if (vclint_lvtt_tsc_deadline(vclint)) {
		timer = &vclint->vtimer.timer;
		del_timer(timer);

		if (val != 0UL) {
			val -= exec_vmread64(HX_TSC_OFFSET_FULL);
			timer->fire_tick = val;
			(void)add_timer(timer);
		} else {
			timer->fire_tick = 0UL;
		}
	} else {
		/* No action required */
	}
}

static void vclint_accept_intr(struct acrn_vclint *vclint, uint32_t vector)
{
	struct clint_regs *clint;
	struct acrn_vcpu *vcpu;

	clint = &(vclint->clint_page);
	vcpu = vector2vcpu(vector);
	signal_event(&(vcpu->events[VCPU_EVENT_VIRTUAL_INTERRUPT]));
	vcpu_make_request(vcpu, ACRN_REQUEST_EVENT);
}

static inline uint32_t
lvt_off_to_idx(uint32_t offset)
{
	uint32_t index;

	switch (offset) {
	case CLINT_OFFSET_MSIP0:
		index = CLINT_LVT_MSIP0;
		break;
	case CLINT_OFFSET_MSIP1:
		index = CLINT_LVT_MSIP1;
		break;
	default:
		index = CLINT_LVT_ERROR;
		break;
	}

	return index;
}

static inline uint32_t *
vclint_get_lvtptr(struct acrn_vclint *vclint, uint32_t offset)
{
	struct clint_regs *clint = &(vclint->clint_page);
	uint32_t i;
	uint32_t *lvt_ptr;

	switch (offset) {
	default:
		i = lvt_off_to_idx(offset);
		lvt_ptr = clint + i;
		break;
	}
	return lvt_ptr;
}

static inline uint32_t
vclint_get_lvt(const struct acrn_vclint *vclint, uint32_t offset)
{
	uint32_t idx;

	idx = lvt_off_to_idx(offset);
	return vclint->lvt_last[idx];
}

static void
vclint_write_lvt(struct acrn_vclint *vclint, uint32_t offset)
{
	uint32_t *lvtptr, mask, val, idx;
	struct clint_regs *clint;
	bool error = false;

	clint = &(vclint->clint_page);
	lvtptr = vclint_get_lvtptr(vclint, offset);
	val = *lvtptr;

	if (offset == CLINT_OFFSET_TIMER0) {
		vclint_update_lvtt(vclint, val);
	} else {
		/* No action required. */
	}

	if (error == false) {
		*lvtptr = val;
		idx = lvt_off_to_idx(offset);
		vclint->lvt_last[idx] = val;
	}
}

static void
vclint_fire_lvt(struct acrn_vclint *vclint, uint32_t lvt)
{
	if ((lvt & CLINT_LVT_M) == 0U) {
		struct acrn_vcpu *vcpu = vclint2vcpu(vclint);
		uint32_t vec = lvt & CLINT_LVT_VECTOR;
		uint32_t mode = lvt & CLINT_LVT_DM;

		switch (mode) {
		case CLINT_LVT_DM_FIXED:
			vclint_set_intr(vcpu, vec);
			break;
		case CLINT_LVT_DM_NMI:
			vcpu_inject_nmi(vcpu);
			break;
		case CLINT_LVT_DM_EXTINT:
			vcpu_inject_extint(vcpu);
			break;
		default:
			/* Other modes ignored */
			pr_warn("func:%s other mode is not support\n",__func__);
			break;
		}
	}
	return;
}

static int32_t
vclint_trigger_lvt(struct acrn_vclint *vclint, uint32_t lvt_index)
{
	uint32_t lvt;
	int32_t ret = 0;

	if (vclint_enabled(vclint)) {
		switch (lvt_index) {
		case CLINT_LVT_MSIP0:
			lvt = vclint_get_lvt(vclint, CLINT_OFFSET_MSIP0);
			break;
		case CLINT_LVT_MSIP1:
			lvt = vclint_get_lvt(vclint, CLINT_OFFSET_MSIP1);
			break;
		default:
			lvt = 0U; /* make MISRA happy */
			ret =  -EINVAL;
			break;
		}

		if (ret == 0) {
			vclint_fire_lvt(vclint, lvt);
		}
	}
	return ret;
}

static inline void set_dest_mask_phys(struct acrn_vm *vm, uint64_t *dmask, uint32_t dest)
{
	uint16_t vcpu_id;

	vcpu_id = dest;
	if (vcpu_id < vm->hw.created_vcpus) {
		bitmap_set_nolock(vcpu_id, dmask);
	}
}

void
vclint_calc_dest(struct acrn_vm *vm, uint64_t *dmask, bool is_broadcast,
		uint32_t dest, bool phys, bool lowprio)
{
	struct acrn_vclint *vclint, *lowprio_dest = NULL;
	struct acrn_vcpu *vcpu;
	uint16_t vcpu_id;

	*dmask = 0UL;
	if (is_broadcast) {
		*dmask = vm_active_cpus(vm);
	} else if (phys) {
		set_dest_mask_phys(vm, dmask, dest);
	} else {
		foreach_vcpu(vcpu_id, vm, vcpu) {
			vclint = vm_clint_from_vcpu_id(vm, vcpu_id);
			bitmap_set_nolock(vcpu_id, dmask);
		}
	}
}

static bool vclint_find_deliverable_intr(const struct acrn_vclint *vclint, uint32_t *vecptr)
{
	const struct clint_regs *clint = &(vclint->clint_page);
	uint32_t vec = 0;
	bool ret = true;

	*vecptr = vec;

	return ret;
}

static void vclint_get_deliverable_intr(struct acrn_vclint *vclint, uint32_t vector)
{
}

static int32_t vclint_read(struct acrn_vclint *vclint, uint32_t offset_arg, uint64_t *data)
{
	int32_t ret = 0;
	struct clint_regs *clint = &(vclint->clint_page);
	uint32_t i;
	uint32_t offset = offset_arg;
	*data = 0UL;

	if (offset > sizeof(*clint)) {
		ret = -EACCES;
	} else {
		offset &= ~0x3UL;
		switch (offset) {
		case CLINT_OFFSET_TIMER0:
		case CLINT_OFFSET_TIMER1:
		case CLINT_OFFSET_TIMER2:
		case CLINT_OFFSET_TIMER3:
		case CLINT_OFFSET_TIMER4:
			*data = *(uint64_t *)(clint + offset);
			break;
		case CLINT_OFFSET_MSIP0:
		case CLINT_OFFSET_MSIP1:
		case CLINT_OFFSET_MSIP2:
		case CLINT_OFFSET_MSIP3:
		case CLINT_OFFSET_MSIP4:
			*data = vclint_get_lvt(vclint, offset);
			break;
		case CLINT_OFFSET_MTIME:
			/* if TSCDEADLINE mode always return 0*/
			if (vclint_lvtt_tsc_deadline(vclint)) {
				*data = 0UL;
			} else {
				*data = clint->mtime;
			}
			break;
		default:
			ret = -EACCES;
			break;
		}
	}

	dev_dbg(DBG_LEVEL_VCLINT, "vclint read offset %x, data %lx", offset, *data);
	return ret;
}

static int32_t vclint_write(struct acrn_vclint *vclint, uint32_t offset, uint64_t data)
{
	struct clint_regs *clint = &(vclint->clint_page);
	uint32_t *regptr;
	uint32_t data32 = (uint32_t)data;
	int32_t ret = 0;

	dev_dbg(DBG_LEVEL_VCLINT, "vclint write offset %#x, data %#lx", offset, data);

	if (offset <= sizeof(*clint)) {
		switch (offset) {
		case  CLINT_OFFSET_MSIP0:
		case  CLINT_OFFSET_MSIP1:
		case  CLINT_OFFSET_MSIP2:
		case  CLINT_OFFSET_MSIP3:
		case  CLINT_OFFSET_MSIP4:
			regptr = vclint_get_lvtptr(vclint, offset);
			*regptr = data32;
			vclint_write_lvt(vclint, offset);
			break;
		case CLINT_OFFSET_TIMER0:
		case CLINT_OFFSET_TIMER1:
		case CLINT_OFFSET_TIMER2:
		case CLINT_OFFSET_TIMER3:
		case CLINT_OFFSET_TIMER4:
			/* if TSCDEADLINE mode ignore icr_timer */
			if (vclint_lvtt_tsc_deadline(vclint)) {
				break;
			}
			vclint_write_tmr(vclint);
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

void
vclint_reset(struct acrn_vclint *vclint, const struct acrn_apicv_ops *ops, enum reset_mode mode)
{
	struct clint_regs *clint;
	uint64_t preserved_clint_mode = vclint->clint_base;

	vclint->clint_base = DEFAULT_CLINT_BASE;

	if (mode == INIT_RESET) {
		vclint->clint_base |= preserved_clint_mode;
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
	uint64_t changed;
	bool change_in_vclint_mode = false;

	if (vclint->clint_base != new) {
		vclint->clint_base = new;
	}

	return ret;
}

void
vclint_set_intr(struct acrn_vcpu *vcpu, uint32_t vector)
{
	struct acrn_vclint *vclint;

	vclint = vcpu_vclint(vcpu);
	vclint_accept_intr(vclint, vector);
}

int32_t
vclint_set_local_intr(struct acrn_vm *vm, uint16_t vcpu_id_arg, uint32_t lvt_index)
{
	struct acrn_vclint *vclint;
	uint64_t dmask = 0UL;
	int32_t error;
	uint16_t vcpu_id = vcpu_id_arg;

	if ((vcpu_id != BROADCAST_CPU_ID) && (vcpu_id >= vm->hw.created_vcpus)) {
	        error = -EINVAL;
	} else {
		if (vcpu_id == BROADCAST_CPU_ID) {
			dmask = vm_active_cpus(vm);
		} else {
			bitmap_set_nolock(vcpu_id, &dmask);
		}
		error = 0;
		for (vcpu_id = 0U; vcpu_id < vm->hw.created_vcpus; vcpu_id++) {
			if ((dmask & (1UL << vcpu_id)) != 0UL) {
				vclint = vm_clint_from_vcpu_id(vm, vcpu_id);
				error = vclint_trigger_lvt(vclint, lvt_index);
				if (error != 0) {
					break;
				}
			}
		}
	}

	return error;
}

static void vclint_timer_expired(void *data)
{
	struct acrn_vcpu *vcpu = (struct acrn_vcpu *)data;
	struct acrn_vclint *vclint;
	struct clint_regs *clint;

	vclint = vcpu_vclint(vcpu);
	clint = &(vclint->clint_page);

	if (!vclint_lvtt_masked(vclint)) {
		vclint_set_intr(vcpu, 7);
	}
}

DEFINE_PER_CPU(uint32_t, clint_id);
void vclint_init(struct acrn_vm *vm)
{
	struct acrn_vclint *vclint = &vm->vclint;
	uint64_t *pml4_page = (uint64_t *)vm->arch_vm.s2ptp;

	if (is_sos_vm(vm)) {
		s2pt_del_mr(vm, pml4_page,
			DEFAULT_CLINT_BASE, PAGE_SIZE);
	}

	s2pt_add_mr(vm, pml4_page,
		vclint_get_clint_access_addr(),
		DEFAULT_CLINT_BASE, PAGE_SIZE,
		EPT_WR | EPT_RD | EPT_UNCACHED);

	vclint_init_timer(vclint);
}

void vclint_free(struct acrn_vcpu *vcpu)
{
	struct acrn_vclint *vclint = vcpu_vclint(vcpu);

	del_timer(&vclint->vtimer.timer);

}

uint64_t
vclint_get_clint_access_addr(void)
{
	static uint8_t apicv_apic_access_addr[PAGE_SIZE] __aligned(PAGE_SIZE);

	return hva2hpa(apicv_apic_access_addr);
}

uint64_t
vclint_get_clint_page_addr(struct acrn_vclint *vclint)
{
	return hva2hpa(&(vclint->clint_page));
}

static void vclint_inject_intr(struct acrn_vclint *vclint,
		bool guest_irq_enabled, bool injected)
{
	uint32_t vector = 0U;

	if (guest_irq_enabled && (!injected)) {
		if (vclint_find_deliverable_intr(vclint, &vector)) {
			exec_vmwrite32(HX_ENTRY_INT_INFO_FIELD, HX_INT_INFO_VALID | vector);
			vclint_get_deliverable_intr(vclint, vector);
		}
	}
}

static bool vclint_has_pending_delivery_intr(struct acrn_vcpu *vcpu)
{
	uint32_t vector;
	struct acrn_vclint *vclint = vcpu_vclint(vcpu);

	if (vclint_find_deliverable_intr(vclint, &vector)) {
		vcpu_make_request(vcpu, ACRN_REQUEST_EVENT);
	}

	return vcpu->arch.pending_req != 0UL;
}

bool vclint_has_pending_intr(struct acrn_vcpu *vcpu)
{
	struct acrn_vclint *vclint = vcpu_vclint(vcpu);
	uint32_t vector;

	return vector != 0UL;
}

static bool vclint_clint_read_access_may_valid(__unused uint32_t offset)
{
	return true;
}

static bool vclint_clint_write_access_may_valid(uint32_t offset)
{
	return true;
}

int32_t clint_access_vmexit_handler(struct acrn_vcpu *vcpu)
{
	int32_t err;
	uint32_t offset;
	uint64_t qual, access_type = TYPE_INST_READ;
	struct acrn_vclint *vclint;
	struct mmio_request *mmio;

	qual = vcpu->arch.exit_qualification;

	if (decode_instruction(vcpu) >= 0) {
		vclint = vcpu_vclint(vcpu);
		offset = qual;
		mmio = &vcpu->req.reqs.mmio;
		if (access_type == TYPE_INST_WRITE) {
			err = emulate_instruction(vcpu);
			if (err == 0) {
				if (vclint->ops->clint_write_access_may_valid(offset)) {
					(void)vclint_write(vclint, offset, mmio->value);
				}
			}
		} else {
			if (vclint->ops->clint_read_access_may_valid(offset)) {
				(void)vclint_read(vclint, offset, &mmio->value);
			} else {
				mmio->value = 0UL;
			}
			err = emulate_instruction(vcpu);
		}
	} else {
		pr_err("%s, unhandled access\n", __func__);
		err = -EINVAL;
	}

	return err;
}

int32_t clint_write_vmexit_handler(struct acrn_vcpu *vcpu)
{
	uint64_t qual;
	int32_t err = 0;
	uint32_t offset;
	struct acrn_vclint *vclint = NULL;

	qual = vcpu->arch.exit_qualification;
	offset = (uint32_t)(qual & 0xFFFUL);

	vcpu_retain_ip(vcpu);
	vclint = vcpu_vclint(vcpu);

	switch (offset) {
	case  CLINT_OFFSET_MSIP0:
	case  CLINT_OFFSET_MSIP1:
	case  CLINT_OFFSET_MSIP2:
	case  CLINT_OFFSET_MSIP3:
	case  CLINT_OFFSET_MSIP4:
		vclint_write_lvt(vclint, offset);
		break;
	case CLINT_OFFSET_TIMER0:
	case CLINT_OFFSET_TIMER1:
	case CLINT_OFFSET_TIMER2:
	case CLINT_OFFSET_TIMER3:
	case CLINT_OFFSET_TIMER4:
		vclint_write_icrtmr(vclint);
		break;
	case CLINT_OFFSET_MTIME:
		vclint_write_dcr(vclint);
		break;
	default:
		err = -EACCES;
		pr_err("Unhandled CLINT-Write, offset:0x%x", offset);
		break;
	}

	return err;
}

static const struct acrn_vclint_ops acrn_vclint_ops = {
	.accept_intr = vclint_accept_intr,
	.inject_intr = vclint_inject_intr,
	.has_pending_delivery_intr = vclint_has_pending_delivery_intr,
	.has_pending_intr = vclint_has_pending_intr,
	.clint_read_access_may_valid = vclint_clint_read_access_may_valid,
	.clint_write_access_may_valid = vclint_clint_write_access_may_valid,
};

void vclint_set_vclint_ops(void)
{
	vclint_ops = &acrn_vclint_ops;
}
