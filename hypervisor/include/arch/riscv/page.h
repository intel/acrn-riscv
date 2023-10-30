#ifndef __RISCV_PAGE_H__
#define __RISCV_PAGE_H__

//#include <public/acrn.h>
//#include <asm/cpu.h>
#include <asm/pgtable.h>

#define PADDR_BITS              48
#define PADDR_MASK              ((1ULL << PADDR_BITS)-1)
#define PAGE_OFFSET(ptr)        ((vaddr_t)(ptr) & ~PAGE_MASK)

#define VADDR_BITS              48
#define VADDR_MASK              0xFFFFFFFFFFFF

/*
 * Memory Type
 */
#define MT_PMA		0x0
#define MT_UC		0x1
#define MT_IO		0x2
#define MT_RSV		0x3

#ifndef __ASSEMBLY__

//#include <acrn/errno.h>
#include <acrn/types.h>
#include <acrn/lib.h>
#include <asm/system.h>

#if defined(CONFIG_RISCV_32)
# include <asm/riscv32/page.h>
#elif defined(CONFIG_RISCV_64)
# include <asm/riscv64/page.h>
#else
# error "unknown RISCV variant"
#endif

/* Architectural minimum cacheline size is 4 32-bit words. */
#define MIN_CACHELINE_BYTES 16
/* Min dcache line size on the boot CPU. */
extern size_t dcache_line_bytes;

#define copy_page(dp, sp) memcpy(dp, sp, PAGE_SIZE)
#define clear_page(page) memset((void *)(page), 0, PAGE_SIZE)


static inline size_t read_dcache_line_bytes(void)
{
    return 128;
}

/* Functions for flushing medium-sized areas.
 * if 'range' is large enough we might want to use model-specific
 * full-cache flushes. */

static inline int invalidate_dcache_va_range(const void *p, unsigned long size)
{
    const void *end = p + size;
    size_t cacheline_mask = dcache_line_bytes - 1;

    dsb(sy);           /* So the CPU issues all writes to the range */

    if ( (uintptr_t)p & cacheline_mask )
    {
        p = (void *)((uintptr_t)p & ~cacheline_mask);
        asm volatile (__clean_and_invalidate_dcache_one(0) : : "r" (p));
        p += dcache_line_bytes;
    }
    if ( (uintptr_t)end & cacheline_mask )
    {
        end = (void *)((uintptr_t)end & ~cacheline_mask);
        asm volatile (__clean_and_invalidate_dcache_one(0) : : "r" (end));
    }

    for ( ; p < end; p += dcache_line_bytes )
        asm volatile (__invalidate_dcache_one(0) : : "r" (p));

    dsb(sy);           /* So we know the flushes happen before continuing */

    return 0;
}

static inline int clean_dcache_va_range(const void *p, unsigned long size)
{
    const void *end = p + size;
    dsb(sy);           /* So the CPU issues all writes to the range */
    p = (void *)((uintptr_t)p & ~(dcache_line_bytes - 1));
    for ( ; p < end; p += dcache_line_bytes )
        asm volatile (__clean_dcache_one(0) : : "r" (p));
    dsb(sy);           /* So we know the flushes happen before continuing */
    /* RISCV callers assume that dcache_* functions cannot fail. */
    return 0;
}

static inline int clean_and_invalidate_dcache_va_range
    (const void *p, unsigned long size)
{
    const void *end = p + size;
    dsb(sy);         /* So the CPU issues all writes to the range */
    p = (void *)((uintptr_t)p & ~(dcache_line_bytes - 1));
    for ( ; p < end; p += dcache_line_bytes )
        asm volatile (__clean_and_invalidate_dcache_one(0) : : "r" (p));
    dsb(sy);         /* So we know the flushes happen before continuing */
    /* RISCV callers assume that dcache_* functions cannot fail. */
    return 0;
}

/* Macros for flushing a single small item.  The predicate is always
 * compile-time constant so this will compile down to 3 instructions in
 * the common case. */
#define clean_dcache(x) do {                                            \
    typeof(x) *_p = &(x);                                               \
    if ( sizeof(x) > MIN_CACHELINE_BYTES || sizeof(x) > alignof(x) )    \
        clean_dcache_va_range(_p, sizeof(x));                           \
    else                                                                \
        asm volatile (                                                  \
            "fence;"   /* Finish all earlier writes */                 \
            __clean_dcache_one(0)                                       \
            "fence;"   /* Finish flush before continuing */            \
            : : "r" (_p), "m" (*_p));                                   \
} while (0)

#define clean_and_invalidate_dcache(x) do {                             \
    typeof(x) *_p = &(x);                                               \
    if ( sizeof(x) > MIN_CACHELINE_BYTES || sizeof(x) > alignof(x) )    \
        clean_and_invalidate_dcache_va_range(_p, sizeof(x));            \
    else                                                                \
        asm volatile (                                                  \
            "dsb sy;"   /* Finish all earlier writes */                 \
            __clean_and_invalidate_dcache_one(0)                        \
            "dsb sy;"   /* Finish flush before continuing */            \
            : : "r" (_p), "m" (*_p));                                   \
} while (0)

/* Flush the dcache for an entire page. */
void flush_page_to_ram(unsigned long mfn, bool sync_icache);

/*
 * Print a walk of a page table or p2m
 *
 * ttbr is the base address register (TTBR0_EL2 or VTTBR_EL2)
 * addr is the PA or IPA to translate
 * root_level is the starting level of the page table
 *   (e.g. TCR_EL2.SL0 or VTCR_EL2.SL0 )
 * nr_root_tables is the number of concatenated tables at the root.
 *   this can only be != 1 for P2M walks starting at the first or
 *   subsequent level.
 */
void dump_pt_walk(paddr_t ttbr, paddr_t addr,
                  unsigned int root_level,
                  unsigned int nr_root_tables);

/* Print a walk of the hypervisor's page tables for a virtual addr. */
extern void dump_hyp_walk(vaddr_t addr);

#endif /* __ASSEMBLY__ */

#define PAGE_ALIGN(x) (((x) + PAGE_SIZE - 1) & PAGE_MASK)

#endif /* __RISCV_PAGE_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
