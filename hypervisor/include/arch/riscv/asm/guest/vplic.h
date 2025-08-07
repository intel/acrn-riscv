/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 *   Haibo1 Xu <haibo1.xu@intel.com>
 */

#ifndef __RISCV_VPLIC_H__
#define __RISCV_VPLIC_H__

#include <asm/page.h>
#include <asm/apicreg.h>

struct acrn_vplic {
	spinlock_t lock;
	struct plic_regs regs;
	struct acrn_vm *vm;
	uint64_t plic_base;
	uint32_t priority_base;
	uint32_t pending_base;
	uint32_t enable_base;
	uint32_t dst_prio_base;
	const struct acrn_vplic_ops *ops;
};

struct acrn_vcpu;
struct acrn_vplic_ops {
	void (*accept_intr)(struct acrn_vplic *vplic, uint32_t vector, bool level);
	void (*inject_intr)(struct acrn_vplic *vplic, bool guest_irq_enabled, bool injected);
	bool (*has_pending_delivery_intr)(struct acrn_vcpu *vcpu);
	bool (*has_pending_intr)(struct acrn_vcpu *vcpu);
	bool (*plic_read_access_may_valid)(uint32_t offset);
	bool (*plic_write_access_may_valid)(uint32_t offset);
};

enum reset_mode;

void vplic_init(struct acrn_vm *vm);
void vplic_accept_intr(struct acrn_vcpu *vcpu, uint32_t vector, bool level);
void vcpu_inject_extint(struct acrn_vcpu *vcpu);

#endif /* __RISCV_VLAPIC_H__ */
