/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_LIB_ATOMIC_H__
#define __RISCV_LIB_ATOMIC_H__

#include <types.h>
#include <asm/system.h>

static inline int32_t atomic_add_return(int32_t i, int32_t *v)
{
	int32_t ret;

	asm volatile (
		"amoadd.w %1, %2, %0\n\t"
		: "+A"(*v), "=r"(ret)
		: "r"(i)
		: "memory"
	);
	smp_mb();
	return ret + i;
}

static inline int64_t atomic_add64_return(int64_t i, int64_t *v)
{
	int64_t ret;

	asm volatile (
		"amoadd.d %1, %2, (%0) \n\t"
		: "+A"(*v), "=r"(ret)
		: "r"(i)
		: "memory"
	);
	smp_mb();
	return ret + i;
}

static inline int32_t atomic_sub_return(int32_t i, int32_t *v)
{
	int32_t ret;

	asm volatile (
		"amoadd.w %1, %2, (%0) \n\t"
		: "+A"(*v), "=r"(ret)
		: "r"(-i)
		: "memory"
	);
	smp_mb();
	return ret - i;
}

static inline int64_t atomic_sub64_return(int64_t i, int64_t *v)
{
	int64_t ret;

	asm volatile (
		"amoadd.d %1, %2, (%0) \n\t"
		: "+A"(*v), "=r"(ret)
		: "r"(-i)
		: "memory"
	);
	smp_mb();
	return ret - i;
}

static inline int64_t atomic_inc64_return(int64_t *v)
{
	return atomic_add64_return(1, v);
}

static inline int64_t atomic_dec64_return(int64_t *v)
{
	return atomic_sub64_return(1, v);
}

static inline int32_t atomic_inc_return(int32_t *v)
{
	return atomic_add_return(1, v);
}

static inline int32_t atomic_dec_return(int32_t *v)
{
	return atomic_sub_return(1, v);
}

#endif /* __RISCV_LIB_ATOMIC_H__ */
