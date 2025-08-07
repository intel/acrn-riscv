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
 * Generic I/O memory access primitives.
 */
static inline void writeb(uint8_t val, volatile void *addr)
{
        asm volatile("sb %0, 0(%1)" : : "r" (val), "r" (addr) : "memory");
}

static inline void writew(uint16_t val, volatile void *addr)
{
        asm volatile("sh %w0, 0(%1)" : : "r" (val), "r" (addr) : "memory");
}

static inline void writel(uint32_t val, volatile void *addr)
{
        asm volatile("sw %0, 0(%1)" : : "r" (val), "r" (addr) : "memory");
}

static inline void writeq(uint64_t val, volatile void *addr)
{
        asm volatile("sd %0, 0(%1)" : : "r" (val), "r" (addr) : "memory");
}

static inline uint8_t readb(const volatile void *addr)
{
        uint8_t val;

        asm volatile("lb %0, 0(%1)": "=r" (val) : "r" (addr) : "memory");
        return val;
}

static inline uint16_t readw(const volatile void *addr)
{
        uint16_t val;

        asm volatile("lh %0, 0(%1)": "=r" (val) : "r" (addr) : "memory");
        return val;
}

static inline uint32_t readl(const volatile void *addr)
{
        uint32_t val;

        asm volatile("lw %0, 0(%1)": "=r" (val) : "r" (addr) : "memory");
        return val;
}

static inline uint64_t readq(const volatile void *addr)
{
        uint64_t val;

        asm volatile("ld %0, 0(%1)": "=r" (val) : "r" (addr) : "memory");
        return val;
}

/* IO barriers */
#define iormb()               rmb()
#define iowmb()               wmb()

/*
 * Strictly ordered I/O memory access primitives.
 */
#define mmio_readb(addr)		({ uint8_t  val = readb(addr); iormb(); val; })
#define mmio_readw(addr)		({ uint16_t val = readw(addr); iormb(); val; })
#define mmio_readl(addr)		({ uint32_t val = readl(addr); iormb(); val; })
#define mmio_readq(addr)		({ uint64_t val = readq(addr); iormb(); val; })

#define mmio_writeb(val, addr)		({ iowmb(); writeb((val), (addr)); })
#define mmio_writew(val, addr)		({ iowmb(); writew((val), (addr)); })
#define mmio_writel(val, addr)		({ iowmb(); writel((val), (addr)); })
#define mmio_writeq(val, addr)		({ iowmb(); writeq((val), (addr)); })

#endif /* __RISCV_IO_H__ */
