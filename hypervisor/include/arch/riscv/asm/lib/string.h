/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_LIB_STRING_H__
#define __RISCV_LIB_STRING_H__

#include <rtl.h>

extern void memcpy(void *d, const void *s, size_t slen);
#define memcpy_erms memcpy
#define memcpy_erms_backwards memcpy

#endif /* __RISCV_STRING_H__ */
