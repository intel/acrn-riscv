/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_ASSIGN_H__
#define __RISCV_ASSIGN_H__

#include <types.h>
#include <ptdev.h>

static void ptirq_intx_ack(struct acrn_vm *vm, uint32_t virt_gsi, enum intx_ctlr vgsi_ctlr){}
static int32_t ptirq_prepare_msix_remap(struct acrn_vm *vm, uint16_t virt_bdf,  uint16_t phys_bdf,
					uint16_t entry_nr, struct msi_info *info, uint16_t irte_idx){}
static int32_t ptirq_intx_pin_remap(struct acrn_vm *vm, uint32_t virt_gsi, enum intx_ctlr vgsi_ctlr){}
static int32_t ptirq_add_intx_remapping(struct acrn_vm *vm, uint32_t virt_gsi, uint32_t phys_gsi, bool pic_pin){}
static void ptirq_remove_intx_remapping(const struct acrn_vm *vm, uint32_t virt_gsi, bool pic_pin, bool is_phy_gsi){}
static void ptirq_remove_msix_remapping(const struct acrn_vm *vm, uint16_t phys_bdf, uint32_t vector_count){}

#endif /* __RISCV_ASSIGN_H__ */
