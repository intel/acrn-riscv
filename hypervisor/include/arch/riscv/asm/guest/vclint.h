/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_VCLINT_H__
#define __RISCV_VCLINT_H__

#include <asm/page.h>
#include <asm/timer.h>
#include <asm/apicreg.h>

#define VCLINT_LVT_MAX	CLINT_LVT_MAX

struct vclint_timer {
	struct hv_timer timer;
	uint32_t tmr_idx;
};

struct acrn_vclint {
	spinlock_t lock;

	struct clint_regs	clint_page;
	struct acrn_vm		*vm;
	struct vclint_timer	vtimer[VCLINT_LVT_MAX];
	uint64_t		mtip;
	uint64_t		clint_base;

	const struct acrn_vclint_ops *ops;
} __aligned(PAGE_SIZE);

struct acrn_vcpu;
struct acrn_vclint_ops {
	bool (*has_pending_delivery_intr)(struct acrn_vcpu *vcpu);
	bool (*has_pending_intr)(struct acrn_vcpu *vcpu);
	bool (*clint_read_access_may_valid)(uint32_t offset);
	bool (*clint_write_access_may_valid)(uint32_t offset);
};

enum reset_mode;
extern const struct acrn_vclint_ops *vclint_ops;

extern void vclint_set_vclint_ops(void);
extern uint64_t vclint_get_tsc_deadline_csr(const struct acrn_vclint*vclint);
extern void vclint_set_tsc_deadline_csr(struct acrn_vclint*vclint, uint32_t index, uint64_t val_arg);
extern uint64_t vclint_get_clintbase(const struct acrn_vclint*vclint);
extern int32_t vclint_set_clintbase(struct acrn_vclint*vclint, uint64_t new);
extern void vclint_set_intr(struct acrn_vcpu *vcpu);
extern void vclint_init(struct acrn_vm *vm);
extern void vclint_free(struct acrn_vcpu *vcpu);
extern void vclint_reset(struct acrn_vclint*vclint, const struct acrn_vclint_ops *ops, enum reset_mode mode);
extern uint64_t vclint_get_clint_access_addr(void);
extern uint64_t vclint_get_clint_page_addr(struct acrn_vclint*vclint);
extern bool vclint_has_pending_intr(struct acrn_vcpu *vcpu);
extern void vclint_send_ipi(struct acrn_vclint *vclint, uint32_t cpu);
extern void vclint_write_tmr(struct acrn_vclint *vclint, uint32_t index, uint64_t data);
#endif /* __RISCV_VCLINT_H__ */
