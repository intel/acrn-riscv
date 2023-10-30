#ifndef __ASM_RISCV_FLUSHTLB_H__
#define __ASM_RISCV_FLUSHTLB_H__


# include <asm/riscv64/flushtlb.h>


/*
 * Flush a range of VA's hypervisor mappings from the TLB of the local
 * processor.
 */
static inline void flush_acrn_tlb_range_va_local(vaddr_t va,
                                                unsigned long size)
{
    vaddr_t end = va + size;

    dsb(sy); /* Ensure preceding are visible */
    while ( va < end )
    {
        __flush_acrn_tlb_one_local(va);
        va += PAGE_SIZE;
    }
    dsb(sy); /* Ensure completion of the TLB flush */
    isb();
}

/*
 * Flush a range of VA's hypervisor mappings from the TLB of all
 * processors in the inner-shareable domain.
 */
static inline void flush_acrn_tlb_range_va(vaddr_t va,
                                          unsigned long size)
{
    vaddr_t end = va + size;

    dsb(sy); /* Ensure preceding are visible */
    while ( va < end )
    {
        __flush_acrn_tlb_one(va);
        va += PAGE_SIZE;
    }
    dsb(sy); /* Ensure completion of the TLB flush */
    isb();
}

#endif /* __ASM_RISCV_FLUSHTLB_H__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
