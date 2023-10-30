/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#ifndef __aligned
#define __aligned(x)		__attribute__((aligned(x)))
#endif
#ifndef __packed
#define __packed	__attribute__((packed))
#endif
#define	__unused	__attribute__((unused))

#ifndef ASSEMBLER

/* Define standard data types.  These definitions allow software components
 * to perform in the same manner on different target platforms.
 */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef signed long int64_t;
typedef unsigned int size_t;
typedef __builtin_va_list va_list;
#ifndef bool
#define bool _Bool
#endif
#ifndef NULL
#define         NULL                                ((void *) 0)
#endif

#ifndef true
#define true		((_Bool) 1)
#define false		((_Bool) 0)
#endif

#define INT8_MIN        (-127-1)
#define INT16_MIN       (-32767-1)
#define INT32_MIN       (-2147483647-1)

#define INT8_MAX        (127)
#define INT16_MAX       (32767)
#define INT32_MAX       (2147483647)

#define UINT8_MAX       (255)
#define UINT16_MAX      (65535)

#define INT_MAX         ((int)(~0U>>1))
#define INT_MIN         (-INT_MAX - 1)
#define UINT_MAX        (~0U)
#define LONG_MAX        ((long)(~0UL>>1))
#define LONG_MIN        (-LONG_MAX - 1)
#define ULONG_MAX       (~0UL)
#define UINT64_MAX	(0xffffffffffffffffUL)
#define UINT32_MAX	(0xffffffffU)

#if defined(CONFIG_ARM) || defined(CONFIG_RISCV)
#define IS_ENABLED(option) ((option) == 1)

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#if defined(CONFIG_ARM_32) || defined(CONFIG_RISCV_32)
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#elif defined (CONFIG_ARM_64) || defined(CONFIG_RISCV_64)
typedef __signed__ long __s64;
typedef unsigned long __u64;
#endif

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

#if defined(CONFIG_ARM_32) || defined(CONFIG_RISCV_32)
typedef signed long long s64;
typedef unsigned long long u64;
typedef u32 vaddr_t;
#define PRIvaddr PRIx32
typedef u64 paddr_t;
#define INVALID_PADDR (~0ULL)
#define PRIpaddr "016llx"
typedef u32 register_t;
#define PRIregister "08x"
#elif defined (CONFIG_ARM_64) || defined(CONFIG_RISCV_64)
typedef signed long s64;
typedef unsigned long u64;
typedef u64 vaddr_t;
#define PRIvaddr PRIx64
typedef u64 paddr_t;
#define INVALID_PADDR (~0UL)
#define PRIpaddr "016lx"
typedef u64 register_t;
#define PRIregister "016lx"
#endif

typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef unsigned int __attribute__((__mode__(__pointer__))) uintptr_t;

typedef bool bool_t;
#define test_and_set_bool(b)   xchg(&(b), true)
#define test_and_clear_bool(b) xchg(&(b), false)

#define BITS_TO_LONGS(bits) \
    (((bits)+BITS_PER_LONG-1)/BITS_PER_LONG)
#define DECLARE_BITMAP(name,bits) \
    unsigned long name[BITS_TO_LONGS(bits)]

#endif

#endif /* ASSEMBLER */

#endif /* INCLUDE_TYPES_H defined */
