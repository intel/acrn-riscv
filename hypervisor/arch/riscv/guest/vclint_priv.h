/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_VCLINT_PRIV_H__
#define __RISCV_VCLINT_PRIV_H__

/*
 * CLINT Register:		Offset	Description
 */
#define CLINT_OFFSET_MSIP0	0x00U	/* msip for hart 0 */
#define CLINT_OFFSET_MSIP1	0x04U	/* msip for hart 1 */
#define CLINT_OFFSET_MSIP2	0x08U	/* msip for hart 2 */
#define CLINT_OFFSET_MSIP3	0x0CU	/* msip for hart 3 */
#define CLINT_OFFSET_MSIP4	0x10U	/* msip for hart 4 */
#define CLINT_OFFSET_TIMER0	0x4000U	/* mtimecmp for hart 0 */
#define CLINT_OFFSET_TIMER1	0x4008U	/* mtimecmp for hart 1 */
#define CLINT_OFFSET_TIMER2	0x4010U	/* mtimecmp for hart 2 */
#define CLINT_OFFSET_TIMER3	0x4018U	/* mtimecmp for hart 3 */
#define CLINT_OFFSET_TIMER4	0x4020U	/* mtimecmp for hart 4 */
#define CLINT_OFFSET_MTIME	0xBFF8U	/* mtime */

#endif /* __RISCV_VCLINT_PRIV_H__ */
