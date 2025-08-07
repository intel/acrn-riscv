/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_BOOT_H__
#define __RISCV_BOOT_H__

#include <asm/types.h>

/* Conforming to OpenSBI dynamic info structure passed by previous booting stage */
struct fw_dynamic_info {
	/* magic */
	uint64_t magic;
	/* version */
	uint64_t version;
	/* next stage entry */
	uint64_t addr;
	/* next stage mode */
	uint64_t mode;
	uint64_t options;
	/* prefered boot hart */
	uint64_t hart_id;
} __packed;

extern struct fw_dynamic_info *fw_dinfo;
extern uint64_t fw_dtb;

#endif /* __RISCV_BOOT_H__ */
