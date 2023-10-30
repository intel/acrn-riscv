/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#include <asm/cpu.h>
	.text

	.align 8
	.global arch_switch_to
arch_switch_to:
	cpu_ctx_save
	sd sp, 0(a0)
	ld sp, 0(a1)
	cpu_ctx_restore
	ret
