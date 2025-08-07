/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_IOAPIC_H__
#define __RISCV_IOAPIC_H__

#include <asm/apicreg.h>

#define CONFIG_MAX_IOAPIC_NUM	1

struct ioapic_info {
	uint8_t id;		/* IOAPIC ID as indicated in ACPI MADT */
	uint32_t addr;		/* IOAPIC Register address */
	uint32_t gsi_base;	/* Global System Interrupt where this IO-APIC's interrupt input start */
	uint32_t nr_pins;	/* Number of Interrupt inputs as determined by Max. Redir Entry Register */
};

static void ioapic_get_rte(uint32_t irq, union ioapic_rte *rte) {}

#endif /* __RISCV_IOAPIC_H__ */
