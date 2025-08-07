/*
 * Copyright (C) 2023-2024 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Authors:
 *   Haicheng Li <haicheng.li@intel.com>
 */

#ifndef __RISCV_IO_H__
#define __RISCV_IO_H__

#include <asm/system.h>

/*
 * Generic IO read/write.  These perform native-endian accesses.
 */
static inline void __raw_writeb(uint8_t val, volatile void *addr)
{
        asm volatile("sb %0, 0(%1)" : : "r" (val), "r" (addr) : "memory");
}

static inline void __raw_writew(uint16_t val, volatile void *addr)
{
        asm volatile("sh %w0, 0(%1)" : : "r" (val), "r" (addr) : "memory");
}

static inline void __raw_writel(uint32_t val, volatile void *addr)
{
        asm volatile("sw %0, 0(%1)" : : "r" (val), "r" (addr) : "memory");
}

static inline void __raw_writeq(uint64_t val, volatile void *addr)
{
        asm volatile("sd %0, 0(%1)" : : "r" (val), "r" (addr) : "memory");
}

static inline uint8_t __raw_readb(const volatile void *addr)
{
        uint8_t val;

        asm volatile("lb %0, 0(%1)": "=r" (val) : "r" (addr) : "memory");
        return val;
}

static inline uint16_t __raw_readw(const volatile void *addr)
{
        uint16_t val;

        asm volatile("lh %0, 0(%1)": "=r" (val) : "r" (addr) : "memory");
        return val;
}

static inline uint32_t __raw_readl(const volatile void *addr)
{
        uint32_t val;

        asm volatile("lw %0, 0(%1)": "=r" (val) : "r" (addr) : "memory");
        return val;
}

static inline uint64_t __raw_readq(const volatile void *addr)
{
        uint64_t val;

        asm volatile("ld %0, 0(%1)": "=r" (val) : "r" (addr) : "memory");
        return val;
}

/* IO barriers */
#define __iormb()               rmb()
#define __iowmb()               wmb()

#define mmiowb()                do { } while (0)

/*
 * Relaxed I/O memory access primitives. These follow the Device memory
 * ordering rules but do not guarantee any ordering relative to Normal memory
 * accesses.
 */
#define readb_relaxed(c)        ({ uint8_t  __v = __raw_readb(c); __v; })
#define readw_relaxed(c)        ({ uint16_t __v = (uint16_t)__raw_readw(c); __v; })
#define readl_relaxed(c)        ({ uint32_t __v = (uint32_t)__raw_readl(c); __v; })
#define readq_relaxed(c)        ({ uint64_t __v = (uint64_t)__raw_readq(c); __v; })

#define writeb_relaxed(v,c)     ((void)__raw_writeb((v),(c)))
#define writew_relaxed(v,c)     ((void)__raw_writew((uint16_t)v,(c)))
#define writel_relaxed(v,c)     ((void)__raw_writel((uint32_t)v,(c)))
#define writeq_relaxed(v,c)     ((void)__raw_writeq((uint64_t)v,(c)))

/*
 * I/O memory access primitives. Reads are ordered relative to any
 * following Normal memory access. Writes are ordered relative to any prior
 * Normal memory access.
 */
#define readb(c)                ({ uint8_t  __v = readb_relaxed(c); __iormb(); __v; })
#define readw(c)                ({ uint16_t __v = readw_relaxed(c); __iormb(); __v; })
#define readl(c)                ({ uint32_t __v = readl_relaxed(c); __iormb(); __v; })
#define readq(c)                ({ uint64_t __v = readq_relaxed(c); __iormb(); __v; })

#define writeb(v,c)             ({ __iowmb(); writeb_relaxed((v),(c)); })
#define writew(v,c)             ({ __iowmb(); writew_relaxed((v),(c)); })
#define writel(v,c)             ({ __iowmb(); writel_relaxed((v),(c)); })
#define writeq(v,c)             ({ __iowmb(); writeq_relaxed((v),(c)); })

#endif /* __RISCV_IO_H__ */
