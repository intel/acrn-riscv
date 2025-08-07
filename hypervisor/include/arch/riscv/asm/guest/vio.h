/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_VIO_H__
#define __RISCV_VIO_H__

#include <types.h>

/* Define emulated port IO index */
#define PIC_MASTER_PIO_IDX		0U
#define PIC_SLAVE_PIO_IDX		(PIC_MASTER_PIO_IDX + 1U)
#define PIC_ELC_PIO_IDX			(PIC_SLAVE_PIO_IDX + 1U)
#define PCI_CFGADDR_PIO_IDX		(PIC_ELC_PIO_IDX + 1U)
#define PCI_CFGDATA_PIO_IDX		(PCI_CFGADDR_PIO_IDX + 1U)
/* When MAX_VUART_NUM_PER_VM is larger than 2, UART_PIO_IDXn should also be added here */
#define UART_PIO_IDX0			(PCI_CFGDATA_PIO_IDX + 1U)
#define UART_PIO_IDX1			(UART_PIO_IDX0 + 1U)
#define PM1A_EVT_PIO_IDX		(UART_PIO_IDX1 + 1U)
#define PM1A_CNT_PIO_IDX		(PM1A_EVT_PIO_IDX + 1U)
#define PM1B_EVT_PIO_IDX		(PM1A_CNT_PIO_IDX + 1U)
#define PM1B_CNT_PIO_IDX		(PM1B_EVT_PIO_IDX + 1U)
#define RTC_PIO_IDX			(PM1B_CNT_PIO_IDX + 1U)
#define VIRTUAL_PM1A_CNT_PIO_IDX	(RTC_PIO_IDX + 1U)
#define KB_PIO_IDX			(VIRTUAL_PM1A_CNT_PIO_IDX + 1U)
#define CF9_PIO_IDX			(KB_PIO_IDX + 1U)
#define PIO_RESET_REG_IDX		(CF9_PIO_IDX + 1U)
#define EMUL_PIO_IDX_MAX		(PIO_RESET_REG_IDX + 1U)

extern int32_t mmio_access_vmexit_handler(struct acrn_vcpu *vcpu);
extern void emulate_pio_complete(struct acrn_vcpu *vcpu, const struct io_request *io_req);
extern void allow_guest_pio_access(struct acrn_vm *vm, uint16_t port_address, uint32_t nbytes);
extern void deny_guest_pio_access(struct acrn_vm *vm, uint16_t port_address, uint32_t nbytes);
extern void arch_fire_hsm_interrupt(void);

#endif /* __RISCV_VIO_H__ */
