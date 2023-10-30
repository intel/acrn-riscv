#ifndef __RISCV_RISCV64_PAGE_H__
#define __RISCV_RISCV64_PAGE_H__

#ifndef __ASSEMBLY__

/* Inline ASM to invalidate dcache on register R (may be an inline asm operand) */
#define __invalidate_dcache_one(R) "fence;"

/* Inline ASM to flush dcache on register R (may be an inline asm operand) */
/*
#define __clean_dcache_one(R)                   \
    ALTERNATIVE("dc cvac, %" #R ";",            \
                "dc civac, %" #R ";",           \
                RISCV64_WORKAROUND_CLEAN_CACHE)   \
*/

#define __clean_dcache_one(R) "fence;"

/* Inline ASM to clean and invalidate dcache on register R (may be an
 * inline asm operand) */
#define __clean_and_invalidate_dcache_one(R) "fence;"

/* Invalidate all instruction caches in Inner Shareable domain to PoU */
static inline void invalidate_icache(void)
{
    asm volatile ("fence_i");
    dsb(ish);               /* Ensure completion of the flush I-cache */
    isb();
}

/* Invalidate all instruction caches on the local processor to PoU */
static inline void invalidate_icache_local(void)
{
    asm volatile ("fence_i");
    dsb(nsh);               /* Ensure completion of the I-cache flush */
    isb();
}

extern void clear_page(void *to);

#endif /* __ASSEMBLY__ */

#endif /* __RISCV_RISCV64_PAGE_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
