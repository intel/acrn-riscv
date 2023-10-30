/******************************************************************************
 * arch-riscv.h
 *
 * Guest OS interface to RISCV acrn.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later
 * Copyright (c) 2023 Intel Corporation
 * All rights reserved.
 *
 */
 
#ifndef __ACRN_PUBLIC_ARCH_RISCV_H__
#define __ACRN_PUBLIC_ARCH_RISCV_H__

#define ACRN_HYPERCALL_TAG   0XEA1

#define int64_aligned_t  int64_t __attribute__((aligned(8)))
#define uint64_aligned_t uint64_t __attribute__((aligned(8)))

#ifndef __ASSEMBLY__
#define ___DEFINE_ACRN_GUEST_HANDLE(name, type)                  \
    typedef union { type *p; unsigned long q; }                 \
        __guest_handle_ ## name;                                \
    typedef union { type *p; uint64_aligned_t q; }              \
        __guest_handle_64_ ## name

/*
 * ACRN_GUEST_HANDLE represents a guest pointer, when passed as a field
 * in a struct in memory. On RISCV is always 8 bytes sizes and 8 bytes
 * aligned.
 * ACRN_GUEST_HANDLE_PARAM represents a guest pointer, when passed as an
 * hypercall argument. It is 4 bytes on aarch32 and 8 bytes on aarch64.
 */
#define __DEFINE_ACRN_GUEST_HANDLE(name, type) \
    ___DEFINE_ACRN_GUEST_HANDLE(name, type);   \
    ___DEFINE_ACRN_GUEST_HANDLE(const_##name, const type)
#define DEFINE_ACRN_GUEST_HANDLE(name)   __DEFINE_ACRN_GUEST_HANDLE(name, name)
#define __ACRN_GUEST_HANDLE(name)        __guest_handle_64_ ## name
#define ACRN_GUEST_HANDLE(name)          __ACRN_GUEST_HANDLE(name)
#define ACRN_GUEST_HANDLE_PARAM(name)    __guest_handle_ ## name
#define set_acrn_guest_handle_raw(hnd, val)                  \
    do {                                                    \
        typeof(&(hnd)) _sxghr_tmp = &(hnd);                 \
        _sxghr_tmp->q = 0;                                  \
        _sxghr_tmp->p = val;                                \
    } while ( 0 )
#define set_acrn_guest_handle(hnd, val) set_acrn_guest_handle_raw(hnd, val)

typedef uint64_t acrn_pfn_t;
#define PRI_acrn_pfn PRIx64
#define PRIu_acrn_pfn PRIu64

/*
 * Maximum number of virtual CPUs in legacy multi-processor guests.
 * Only one. All other VCPUS must use VCPUOP_register_vcpu_info.
 */
#define ACRN_LEGACY_MAX_VCPUS 1

typedef uint64_t acrn_ulong_t;
#define PRI_acrn_ulong PRIx64

#if defined(__ACRN__) || defined(__ACRN_TOOLS__)
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
/* Anonymous union includes both 32- and 64-bit names (e.g., r0/x0). */
# define __DECL_REG(n64, n32) union {          \
        uint64_t n64;                          \
        uint32_t n32;                          \
    }
#else
/* Non-gcc sources must always use the proper 64-bit name (e.g., x0). */
#define __DECL_REG(n64, n32) uint64_t n64
#endif

struct vcpu_guest_core_regs
{
    /*         Aarch64       Aarch32 */
    __DECL_REG(x0,           r0_usr);
    __DECL_REG(x1,           r1_usr);
    __DECL_REG(x2,           r2_usr);
    __DECL_REG(x3,           r3_usr);
    __DECL_REG(x4,           r4_usr);
    __DECL_REG(x5,           r5_usr);
    __DECL_REG(x6,           r6_usr);
    __DECL_REG(x7,           r7_usr);
    __DECL_REG(x8,           r8_usr);
    __DECL_REG(x9,           r9_usr);
    __DECL_REG(x10,          r10_usr);
    __DECL_REG(x11,          r11_usr);
    __DECL_REG(x12,          r12_usr);

    __DECL_REG(x13,          sp_usr);
    __DECL_REG(x14,          lr_usr);

    __DECL_REG(x15,          __unused_sp_hyp);

    __DECL_REG(x16,          lr_irq);
    __DECL_REG(x17,          sp_irq);

    __DECL_REG(x18,          lr_svc);
    __DECL_REG(x19,          sp_svc);

    __DECL_REG(x20,          lr_abt);
    __DECL_REG(x21,          sp_abt);

    __DECL_REG(x22,          lr_und);
    __DECL_REG(x23,          sp_und);

    __DECL_REG(x24,          r8_fiq);
    __DECL_REG(x25,          r9_fiq);
    __DECL_REG(x26,          r10_fiq);
    __DECL_REG(x27,          r11_fiq);
    __DECL_REG(x28,          r12_fiq);

    __DECL_REG(x29,          sp_fiq);
    __DECL_REG(x30,          lr_fiq);

    /* Return address and mode */
    __DECL_REG(pc64,         pc32);             /* ELR_EL2 */
    uint32_t cpsr;                              /* SPSR_EL2 */

    union {
        uint32_t spsr_el1;       /* AArch64 */
        uint32_t spsr_svc;       /* AArch32 */
    };

    /* AArch32 guests only */
    uint32_t spsr_fiq, spsr_irq, spsr_und, spsr_abt;

    /* AArch64 guests only */
    uint64_t sp_el0;
    uint64_t sp_el1, elr_el1;
};
typedef struct vcpu_guest_core_regs vcpu_guest_core_regs_t;
DEFINE_ACRN_GUEST_HANDLE(vcpu_guest_core_regs_t);

#undef __DECL_REG

struct vcpu_guest_context {
#define _VGCF_online                   0
#define VGCF_online                    (1<<_VGCF_online)
    uint32_t flags;                         /* VGCF_* */

    struct vcpu_guest_core_regs user_regs;  /* Core CPU registers */

    uint64_t sctlr;
    uint64_t ttbcr, ttbr0, ttbr1;
};
typedef struct vcpu_guest_context vcpu_guest_context_t;
DEFINE_ACRN_GUEST_HANDLE(vcpu_guest_context_t);

struct acrn_arch_domainconfig {
    /* IN/OUT */
    uint8_t gic_version;
    /* IN */
    uint16_t tee_type;
    /* IN */
    uint32_t nr_spis;
    /*
     * OUT
     * Based on the property clock-frequency in the DT timer node.
     * The property may be present when the bootloader/firmware doesn't
     * set correctly CNTFRQ which hold the timer frequency.
     *
     * As it's not possible to trap this register, we have to replicate
     * the value in the guest DT.
     *
     * = 0 => property not present
     * > 0 => Value of the property
     *
     */
    uint32_t clock_frequency;
};
#endif /* __ACRN__ || __ACRN_TOOLS__ */

struct arch_vcpu_info {
};
typedef struct arch_vcpu_info arch_vcpu_info_t;

struct arch_shared_info {
};
typedef struct arch_shared_info arch_shared_info_t;
typedef uint64_t acrn_callback_t;

#endif

#if defined(__ACRN__) || defined(__ACRN_TOOLS__)

#define GUEST_RAM_BANKS   2

#define GUEST_RAM0_BASE   acrn_mk_ullong(0x40000000) /* 3GB of low RAM @ 1GB */
#define GUEST_RAM0_SIZE   acrn_mk_ullong(0xc0000000)

#define GUEST_RAM1_BASE   acrn_mk_ullong(0x0200000000) /* 1016GB of RAM @ 8GB */
#define GUEST_RAM1_SIZE   acrn_mk_ullong(0xfe00000000)

#define GUEST_RAM_BASE    GUEST_RAM0_BASE /* Lowest RAM address */
/* Largest amount of actual RAM, not including holes */
#define GUEST_RAM_MAX     (GUEST_RAM0_SIZE + GUEST_RAM1_SIZE)
/* Suitable for e.g. const uint64_t ramfoo[] = GUEST_RAM_BANK_FOOS; */
#define GUEST_RAM_BANK_BASES   { GUEST_RAM0_BASE, GUEST_RAM1_BASE }
#define GUEST_RAM_BANK_SIZES   { GUEST_RAM0_SIZE, GUEST_RAM1_SIZE }

/* Current supported guest VCPUs */
#define GUEST_MAX_VCPUS 128

#endif

#endif /*  __ACRN_PUBLIC_ARCH_RISCV_H__ */
