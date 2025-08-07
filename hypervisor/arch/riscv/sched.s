/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#include <asm/cpu.h>

	.text

	.align 8
	.global arch_switch_to
arch_switch_to:

/* refer to struct stack_frame */
	addi sp, sp, -0x80
	sd ra, 0x0(sp)
	sd s0, 0x8(sp)
	sd s1, 0x10(sp)
	sd s2, 0x18(sp)
	sd s3, 0x20(sp)
	sd s4, 0x28(sp)
	sd s5, 0x30(sp)
	sd s6, 0x38(sp)
	sd s7, 0x40(sp)
	sd s8, 0x48(sp)
	sd s9, 0x50(sp)
	sd s10, 0x58(sp)
	sd s11, 0x60(sp)
	sd tp, 0x68(sp)
	sd a0, 0x70(sp)
	sd sp, 0(a0)

	ld sp, 0(a1)
	ld ra, 0x0(sp)
	ld s0, 0x8(sp)
	ld s1, 0x10(sp)
	ld s2, 0x18(sp)
	ld s3, 0x20(sp)
	ld s4, 0x28(sp)
	ld s5, 0x30(sp)
	ld s6, 0x38(sp)
	ld s7, 0x40(sp)
	ld s8, 0x48(sp)
	ld s9, 0x50(sp)
	ld s10, 0x58(sp)
	ld s11, 0x60(sp)
	ld tp, 0x68(sp)
	ld a0, 0x70(sp)
	addi sp, sp, 0x80

	ret
